// renderer/SecondWindow.cpp
#include "renderer/SecondWindow.hpp"
#include "renderer/Renderer.hpp"   // for DrawCall
#include "platform/VulkanContext.hpp"

#include <volk.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>
#include <format>

namespace ndde::renderer {

SecondWindow::~SecondWindow() { destroy(); }

// ── init ──────────────────────────────────────────────────────────────────────

void SecondWindow::init(const platform::VulkanContext& ctx,
                         int x, int y, u32 width, u32 height,
                         const std::string& title,
                         const std::string& shader_dir)
{
    m_instance        = ctx.instance();
    m_physical_device = ctx.physical_device();
    m_device          = ctx.device();
    m_gfx_queue       = ctx.graphics_queue();
    m_present_queue   = ctx.present_queue();
    m_gfx_family      = ctx.queue_families().graphics;

    // GLFW window — glfwInit already called by primary GlfwContext.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED,  GLFW_TRUE);
    glfwWindowHint(GLFW_MAXIMIZED,  GLFW_FALSE);
    m_window = glfwCreateWindow(static_cast<int>(width),
                                static_cast<int>(height),
                                title.c_str(), nullptr, nullptr);
    if (!m_window)
        throw std::runtime_error("[SecondWindow] glfwCreateWindow failed");

    glfwSetWindowPos(m_window, x, y);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, resize_callback);

    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
        throw std::runtime_error("[SecondWindow] glfwCreateWindowSurface failed");

    int fw=0, fh=0;
    glfwGetFramebufferSize(m_window, &fw, &fh);
    build_swapchain(static_cast<u32>(fw), static_cast<u32>(fh));

    create_cmd_objects();
    create_sync_objects();
    init_pipelines(shader_dir);
    m_initialised = true;
    std::cout << "[SecondWindow] Ready: " << title << "\n";
}

// ── destroy ───────────────────────────────────────────────────────────────────

void SecondWindow::destroy() {
    if (!m_initialised) return;
    vkDeviceWaitIdle(m_device);

    m_pipeline_line_list.destroy();
    m_pipeline_line_strip.destroy();
    m_pipeline_triangle_list.destroy();

    if (m_render_fence    != VK_NULL_HANDLE) vkDestroyFence    (m_device, m_render_fence,    nullptr);
    if (m_image_available != VK_NULL_HANDLE) vkDestroySemaphore(m_device, m_image_available, nullptr);
    if (m_render_finished != VK_NULL_HANDLE) vkDestroySemaphore(m_device, m_render_finished, nullptr);
    if (m_cmd_pool        != VK_NULL_HANDLE) vkDestroyCommandPool(m_device, m_cmd_pool, nullptr);

    destroy_swapchain();
    if (m_surface  != VK_NULL_HANDLE) vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    if (m_window) { glfwDestroyWindow(m_window); m_window = nullptr; }

    m_render_fence    = VK_NULL_HANDLE;
    m_image_available  = VK_NULL_HANDLE;
    m_render_finished  = VK_NULL_HANDLE;
    m_cmd_pool     = VK_NULL_HANDLE;
    m_surface      = VK_NULL_HANDLE;
    m_initialised  = false;
}

// ── begin_frame ───────────────────────────────────────────────────────────────

bool SecondWindow::begin_frame() {
    if (!m_initialised) return false;

    vkWaitForFences(m_device, 1, &m_render_fence, VK_TRUE, UINT64_MAX);

    VkResult acq = vkAcquireNextImageKHR(
        m_device, m_sc_raw, UINT64_MAX, m_image_available, VK_NULL_HANDLE, &m_image_index);
    if (acq == VK_ERROR_OUT_OF_DATE_KHR) { on_resize(); return false; }
    if (acq != VK_SUCCESS && acq != VK_SUBOPTIMAL_KHR) return false;

    vkResetFences(m_device, 1, &m_render_fence);
    vkResetCommandBuffer(m_cmd, 0);

    VkCommandBufferBeginInfo begin{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(m_cmd, &begin);

    transition_image(m_sc_images[m_image_index],
                     VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo color_attach{
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView   = m_sc_views[m_image_index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue  = { .color = {{ 0.04f, 0.04f, 0.06f, 1.f }} }
    };
    VkRenderingInfo render_info{
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = { {0,0}, m_sc_extent },
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &color_attach
    };
    vkCmdBeginRendering(m_cmd, &render_info);

    const f32 w = static_cast<f32>(m_sc_extent.width);
    const f32 h = static_cast<f32>(m_sc_extent.height);
    VkViewport vp{ .x=0.f,.y=h,.width=w,.height=-h,.minDepth=0.f,.maxDepth=1.f };
    VkRect2D   sc{ {0,0}, m_sc_extent };
    vkCmdSetViewport(m_cmd, 0, 1, &vp);
    vkCmdSetScissor (m_cmd, 0, 1, &sc);

    m_frame_open = true;
    return true;
}

// ── draw ──────────────────────────────────────────────────────────────────────

void SecondWindow::draw(const DrawCall& dc) {
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
    VkDeviceSize off = static_cast<VkDeviceSize>(dc.slice.byte_offset);
    vkCmdBindVertexBuffers(m_cmd, 0, 1, &dc.slice.buffer, &off);
    vkCmdDraw(m_cmd, dc.slice.vertex_count, 1, 0, 0);
}

// ── end_frame ─────────────────────────────────────────────────────────────────

bool SecondWindow::end_frame() {
    if (!m_frame_open) return false;
    vkCmdEndRendering(m_cmd);
    transition_image(m_sc_images[m_image_index],
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    vkEndCommandBuffer(m_cmd);

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &m_image_available,
        .pWaitDstStageMask    = &wait_stage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &m_cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &m_render_finished
    };
    vkQueueSubmit(m_gfx_queue, 1, &submit, m_render_fence);

    VkPresentInfoKHR present{
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &m_render_finished,
        .swapchainCount     = 1,
        .pSwapchains        = &m_sc_raw,
        .pImageIndices      = &m_image_index
    };
    VkResult pr = vkQueuePresentKHR(m_present_queue, &present);
    m_frame_open = false;
    if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR) { on_resize(); return false; }
    return pr == VK_SUCCESS;
}

// ── on_resize ─────────────────────────────────────────────────────────────────

void SecondWindow::on_resize() {
    int fw=0, fh=0;
    glfwGetFramebufferSize(m_window, &fw, &fh);
    if (fw==0 || fh==0) return;
    vkDeviceWaitIdle(m_device);
    destroy_swapchain();
    build_swapchain(static_cast<u32>(fw), static_cast<u32>(fh));
}

bool SecondWindow::should_close() const noexcept {
    return m_window && glfwWindowShouldClose(m_window);
}

// ── Private: build_swapchain ──────────────────────────────────────────────────

void SecondWindow::build_swapchain(u32 w, u32 h) {
    vkb::SwapchainBuilder builder{ m_physical_device, m_device, m_surface };
    auto sc_ret = builder
        .set_desired_format({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(w, h)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build();
    if (!sc_ret)
        throw std::runtime_error("[SecondWindow] build_swapchain: " + sc_ret.error().message());

    vkb::Swapchain vkb_sc = sc_ret.value();
    m_sc_raw    = vkb_sc.swapchain;
    m_sc_format = vkb_sc.image_format;
    m_sc_extent = vkb_sc.extent;
    m_sc_images = vkb_sc.get_images().value();
    m_sc_views  = vkb_sc.get_image_views().value();
    std::cout << std::format("[SecondWindow] Swapchain {}x{} images:{}\n",
        m_sc_extent.width, m_sc_extent.height, m_sc_images.size());
}

void SecondWindow::destroy_swapchain() {
    for (auto iv : m_sc_views)
        vkDestroyImageView(m_device, iv, nullptr);
    m_sc_views.clear();
    if (m_sc_raw != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device, m_sc_raw, nullptr);
        m_sc_raw = VK_NULL_HANDLE;
    }
}

void SecondWindow::create_cmd_objects() {
    VkCommandPoolCreateInfo ci{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_gfx_family
    };
    vkCreateCommandPool(m_device, &ci, nullptr, &m_cmd_pool);
    VkCommandBufferAllocateInfo ai{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = m_cmd_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers(m_device, &ai, &m_cmd);
}

void SecondWindow::create_sync_objects() {
    VkFenceCreateInfo fi{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    vkCreateFence(m_device, &fi, nullptr, &m_render_fence);
    VkSemaphoreCreateInfo si{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(m_device, &si, nullptr, &m_image_available);
    vkCreateSemaphore(m_device, &si, nullptr, &m_render_finished);
}

void SecondWindow::init_pipelines(const std::string& shader_dir) {
    const std::string vert = shader_dir + "/curve.vert.spv";
    const std::string frag = shader_dir + "/curve.frag.spv";
    m_pipeline_line_list.init    (m_device, m_sc_format, vert, frag, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
    m_pipeline_line_strip.init   (m_device, m_sc_format, vert, frag, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
    m_pipeline_triangle_list.init(m_device, m_sc_format, vert, frag, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
}

void SecondWindow::transition_image(VkImage image, VkImageLayout from, VkImageLayout to) {
    VkImageMemoryBarrier b{
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask       = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
        .oldLayout           = from, .newLayout = to,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image,
        .subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0,1,0,1 }
    };
    vkCmdPipelineBarrier(m_cmd,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr, 1, &b);
}

Pipeline& SecondWindow::pipeline_for(Topology topo) {
    switch (topo) {
        case Topology::LineList:     return m_pipeline_line_list;
        case Topology::LineStrip:    return m_pipeline_line_strip;
        case Topology::TriangleList: return m_pipeline_triangle_list;
    }
    return m_pipeline_line_strip;
}

void SecondWindow::resize_callback(GLFWwindow* win, int, int) {
    auto* self = static_cast<SecondWindow*>(glfwGetWindowUserPointer(win));
    if (self) self->on_resize();
}

} // namespace ndde::renderer
