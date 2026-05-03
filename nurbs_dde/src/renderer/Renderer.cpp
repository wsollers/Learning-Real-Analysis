// renderer/Renderer.cpp
#include "renderer/Renderer.hpp"
#include <stdexcept>
#include <iostream>
#include <format>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace ndde::renderer {

Renderer::~Renderer() { destroy(); }

void Renderer::init(const platform::VulkanContext& ctx,
                    const Swapchain&               swapchain,
                    const std::string&             shader_dir,
                    const std::string&             assets_dir,
                    GLFWwindow*                    window)
{
    m_device         = ctx.device();
    m_graphics_queue = ctx.graphics_queue();
    m_present_queue  = ctx.present_queue();

    create_command_objects(ctx.queue_families().graphics);
    create_sync_objects(swapchain.image_count());
    init_pipelines(swapchain.format(), shader_dir);
    m_imgui.init(window, ctx, swapchain, assets_dir);

    std::cout << "[Renderer] Initialised\n";
}

void Renderer::destroy() {
    if (m_device == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(m_device);

    m_imgui.destroy();
    m_pipeline_line_list.destroy();
    m_pipeline_line_strip.destroy();
    m_pipeline_triangle_list.destroy();

    if (m_render_fence    != VK_NULL_HANDLE) vkDestroyFence(m_device, m_render_fence, nullptr);
    for (auto& sem : m_image_available)
        vkDestroySemaphore(m_device, sem, nullptr);
    m_image_available.clear();
    for (auto& sem : m_render_finished)
        vkDestroySemaphore(m_device, sem, nullptr);
    m_render_finished.clear();
    if (m_cmd_pool        != VK_NULL_HANDLE) vkDestroyCommandPool(m_device, m_cmd_pool, nullptr);

    m_render_fence    = VK_NULL_HANDLE;
    m_cmd_pool        = VK_NULL_HANDLE;
    m_cmd             = VK_NULL_HANDLE;
    m_device          = VK_NULL_HANDLE;
}

bool Renderer::begin_frame(const Swapchain& swapchain) {
    vkWaitForFences(m_device, 1, &m_render_fence, VK_TRUE, UINT64_MAX);

    m_frame_sync = m_sync_index;
    VkSemaphore acquire_sem = m_image_available[m_frame_sync];
    VkResult acquire = vkAcquireNextImageKHR(
        m_device, swapchain.swapchain(), UINT64_MAX,
        acquire_sem, VK_NULL_HANDLE, &m_image_index);

    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) return false;
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("[Renderer] vkAcquireNextImageKHR failed");

    vkResetFences(m_device, 1, &m_render_fence);
    vkResetCommandBuffer(m_cmd, 0);

    // Reset per-frame draw call counter
    m_draw_calls_current = 0;

    VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    if (vkBeginCommandBuffer(m_cmd, &begin_info) != VK_SUCCESS)
        throw std::runtime_error("[Renderer] vkBeginCommandBuffer failed");

    transition_image(swapchain.images()[m_image_index],
                     VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo color_attach{
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView   = swapchain.image_views()[m_image_index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue  = { .color = {{ 0.05f, 0.05f, 0.08f, 1.0f }} }
    };

    VkExtent2D ext = swapchain.extent();
    VkRenderingInfo render_info{
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = { {0, 0}, ext },
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &color_attach
    };
    vkCmdBeginRendering(m_cmd, &render_info);

    // Negative-height VkViewport: aligns Vulkan NDC Y with glm::ortho convention.
    // NDC Y=+1 → screen top,  NDC Y=-1 → screen bottom.
    // This makes world +Y render at the screen top and fixes the epsilon-ball
    // tracking inversion described in docs/COORDINATE_SYSTEMS.md.
    const float w = static_cast<f32>(ext.width);
    const float h = static_cast<f32>(ext.height);
    VkViewport viewport{
        .x = 0.f, .y = h, .width = w, .height = -h,
        .minDepth = 0.f, .maxDepth = 1.f
    };
    vkCmdSetViewport(m_cmd, 0, 1, &viewport);

    VkRect2D scissor{ {0, 0}, ext };
    vkCmdSetScissor(m_cmd, 0, 1, &scissor);

    m_frame_open = true;
    return true;
}

void Renderer::draw(const DrawCall& dc) {
    if (!m_frame_open || !dc.slice.valid()) return;

    Pipeline& pipe = pipeline_for(dc.topology);
    if (!pipe.valid()) return;

    vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.pipeline());

    PushConstants pc{
        .mvp           = dc.mvp,
        .uniform_color = dc.color,
        .mode          = static_cast<i32>(dc.mode)
    };
    vkCmdPushConstants(m_cmd, pipe.layout(),
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(PushConstants), &pc);

    VkDeviceSize offset = static_cast<VkDeviceSize>(dc.slice.byte_offset);
    vkCmdBindVertexBuffers(m_cmd, 0, 1, &dc.slice.buffer, &offset);
    vkCmdDraw(m_cmd, dc.slice.vertex_count, 1, 0, 0);

    ++m_draw_calls_current;
}

bool Renderer::end_frame(const Swapchain& swapchain) {
    // Latch completed frame's draw call count before presenting
    m_draw_calls_last = m_draw_calls_current;

    vkCmdEndRendering(m_cmd);

    transition_image(swapchain.images()[m_image_index],
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    if (vkEndCommandBuffer(m_cmd) != VK_SUCCESS)
        throw std::runtime_error("[Renderer] vkEndCommandBuffer failed");

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSemaphore wait_sem   = m_image_available[m_frame_sync];
    VkSemaphore signal_sem = m_render_finished[m_frame_sync];
    VkSubmitInfo submit{
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &wait_sem,
        .pWaitDstStageMask    = &wait_stage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &m_cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &signal_sem
    };
    if (vkQueueSubmit(m_graphics_queue, 1, &submit, m_render_fence) != VK_SUCCESS)
        throw std::runtime_error("[Renderer] vkQueueSubmit failed");

    VkSwapchainKHR sc = swapchain.swapchain();
    VkPresentInfoKHR present{
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &signal_sem,
        .swapchainCount     = 1,
        .pSwapchains        = &sc,
        .pImageIndices      = &m_image_index
    };

    VkResult pr = vkQueuePresentKHR(m_present_queue, &present);
    m_frame_open = false;
    m_sync_index = (m_sync_index + 1u) % static_cast<u32>(m_image_available.size());

    if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR) return false;
    if (pr != VK_SUCCESS) throw std::runtime_error("[Renderer] vkQueuePresentKHR failed");
    return true;
}

void Renderer::on_swapchain_recreated(const Swapchain& swapchain) {
    m_imgui.on_swapchain_recreated(swapchain);
}

void Renderer::reset_frame_state() {
    // Called after vkDeviceWaitIdle (guaranteed by switch_scene).
    // Destroy and recreate both semaphore vectors so every slot is
    // cleanly unsignaled before the new scene's first acquire.
    const VkSemaphoreCreateInfo si{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    for (auto& sem : m_image_available) {
        if (sem != VK_NULL_HANDLE) vkDestroySemaphore(m_device, sem, nullptr);
        if (vkCreateSemaphore(m_device, &si, nullptr, &sem) != VK_SUCCESS)
            throw std::runtime_error("[Renderer] vkCreateSemaphore (image_available reset) failed");
    }
    for (auto& sem : m_render_finished) {
        if (sem != VK_NULL_HANDLE) vkDestroySemaphore(m_device, sem, nullptr);
        if (vkCreateSemaphore(m_device, &si, nullptr, &sem) != VK_SUCCESS)
            throw std::runtime_error("[Renderer] vkCreateSemaphore (render_finished reset) failed");
    }
    // Recreate the fence in signaled state so begin_frame's WaitForFences
    // returns immediately on the first frame of the new scene.
    const VkFenceCreateInfo fi{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    vkDestroyFence(m_device, m_render_fence, nullptr);
    if (vkCreateFence(m_device, &fi, nullptr, &m_render_fence) != VK_SUCCESS)
        throw std::runtime_error("[Renderer] vkCreateFence (reset) failed");
    m_image_index = 0;
    m_sync_index  = 0;
    m_frame_sync  = 0;
    m_frame_open  = false;
}

void Renderer::create_command_objects(u32 graphics_queue_family) {
    VkCommandPoolCreateInfo pool_info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphics_queue_family
    };
    if (vkCreateCommandPool(m_device, &pool_info, nullptr, &m_cmd_pool) != VK_SUCCESS)
        throw std::runtime_error("[Renderer] vkCreateCommandPool failed");

    VkCommandBufferAllocateInfo alloc_info{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = m_cmd_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    if (vkAllocateCommandBuffers(m_device, &alloc_info, &m_cmd) != VK_SUCCESS)
        throw std::runtime_error("[Renderer] vkAllocateCommandBuffers failed");
}

void Renderer::create_sync_objects(u32 image_count) {
    VkFenceCreateInfo fence_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    if (vkCreateFence(m_device, &fence_info, nullptr, &m_render_fence) != VK_SUCCESS)
        throw std::runtime_error("[Renderer] vkCreateFence failed");

    VkSemaphoreCreateInfo sem_info{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    // One acquire semaphore per swapchain image so each image slot has its
    // own semaphore. This prevents the validation error where the same
    // semaphore is re-signalled by vkAcquireNextImageKHR before presentation
    // has finished consuming it from the previous frame.
    m_image_available.resize(image_count, VK_NULL_HANDLE);
    for (auto& sem : m_image_available)
        if (vkCreateSemaphore(m_device, &sem_info, nullptr, &sem) != VK_SUCCESS)
            throw std::runtime_error("[Renderer] vkCreateSemaphore (image_available) failed");

    m_render_finished.resize(image_count, VK_NULL_HANDLE);
    for (auto& sem : m_render_finished)
        if (vkCreateSemaphore(m_device, &sem_info, nullptr, &sem) != VK_SUCCESS)
            throw std::runtime_error("[Renderer] vkCreateSemaphore (render_finished) failed");
}

void Renderer::init_pipelines(VkFormat color_format, const std::string& shader_dir) {
    const std::string vert = shader_dir + "/curve.vert.spv";
    const std::string frag = shader_dir + "/curve.frag.spv";

    m_pipeline_line_list.init(m_device, color_format, vert, frag,
                               VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
    m_pipeline_line_strip.init(m_device, color_format, vert, frag,
                                VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
    m_pipeline_triangle_list.init(m_device, color_format, vert, frag,
                                   VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    std::cout << "[Renderer] Pipelines: LineList, LineStrip, TriangleList\n";
}

void Renderer::transition_image(VkImage image, VkImageLayout from, VkImageLayout to) {
    VkImageMemoryBarrier barrier{
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask       = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
        .oldLayout           = from,
        .newLayout           = to,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image,
        .subresourceRange    = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };
    vkCmdPipelineBarrier(m_cmd,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
}

Pipeline& Renderer::pipeline_for(Topology topo) {
    switch (topo) {
        case Topology::LineList:     return m_pipeline_line_list;
        case Topology::LineStrip:    return m_pipeline_line_strip;
        case Topology::TriangleList: return m_pipeline_triangle_list;
    }
    return m_pipeline_line_strip;
}

} // namespace ndde::renderer
