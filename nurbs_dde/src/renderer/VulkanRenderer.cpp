#include "renderer/VulkanRenderer.hpp"
#include "app/AppConfig.hpp"
#include <VkBootstrap.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

#include "core/BufferManager.hpp"

namespace ndde {
    struct FrameData {
        VkSemaphore present_semaphore;
        VkSemaphore render_semaphore;
        VkFence render_fence;
    };
    FrameData frames[2]; // Double buffering


    VulkanRenderer::VulkanRenderer(const AppConfig& config) {
        init_window();
        m_context.init(m_window);

        // Swapchain
        vkb::SwapchainBuilder sc_builder{ m_context.vkb_device() };
        auto vkb_sc = sc_builder.build().value();
        m_swapchain             = vkb_sc.swapchain;
        m_swapchain_images      = vkb_sc.get_images().value();
        m_swapchain_image_views = vkb_sc.get_image_views().value();

        // Command pool
        VkCommandPoolCreateInfo pool_info{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = m_context.queue_families().graphics;
        vkCreateCommandPool(m_context.device(), &pool_info, nullptr, &m_command_pool);

        // Command buffer
        VkCommandBufferAllocateInfo alloc_info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        alloc_info.commandPool        = m_command_pool;
        alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;
        vkAllocateCommandBuffers(m_context.device(), &alloc_info, &m_command_buffer);

        create_sync_objects();


        init_pipelines();
    }

    void VulkanRenderer::init_window() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_window = glfwCreateWindow(m_width, m_height, "NDDE Engine", nullptr, nullptr);
    }

    void VulkanRenderer::create_sync_objects() {
        VkFenceCreateInfo fence_info{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };
        vkCreateFence(m_context.device(), &fence_info, nullptr, &m_render_fence);

        VkSemaphoreCreateInfo sem_info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(m_context.device(), &sem_info, nullptr, &m_present_semaphore);
        vkCreateSemaphore(m_context.device(), &sem_info, nullptr, &m_render_semaphore);
    }

    void VulkanRenderer::begin_frame() {
        // This forces the CPU to wait until the GPU is totally idle.
        // It kills performance but stops the validation errors immediately.
        vkDeviceWaitIdle(m_context.device());





        VkDevice dev = m_context.device();

        vkWaitForFences(dev, 1, &m_render_fence, VK_TRUE, UINT64_MAX);
        vkResetFences(dev, 1, &m_render_fence);

        vkAcquireNextImageKHR(dev, m_swapchain, UINT64_MAX,
                              m_present_semaphore, VK_NULL_HANDLE, &m_swapchain_image_index);

        vkResetCommandBuffer(m_command_buffer, 0);

        VkCommandBufferBeginInfo begin_info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(m_command_buffer, &begin_info);

        transition_image(m_command_buffer,
                         m_swapchain_images[m_swapchain_image_index],
                         VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkRenderingAttachmentInfo color_attach{
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = m_swapchain_image_views[m_swapchain_image_index],
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = {{{0.05f, 0.05f, 0.08f, 1.0f}}}
        };

        VkRenderingInfo render_info{
            .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea           = {{0, 0}, {m_width, m_height}},
            .layerCount           = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &color_attach
        };

        vkCmdBeginRendering(m_command_buffer, &render_info);
    }

    void VulkanRenderer::draw_geometry(const AppConfig& config) {
        // (Matrix math remains the same)
    }

    void VulkanRenderer::end_frame() {
        vkCmdEndRendering(m_command_buffer);

        transition_image(m_command_buffer,
                         m_swapchain_images[m_swapchain_image_index],
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        vkEndCommandBuffer(m_command_buffer);

        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.waitSemaphoreCount   = 1;
        submit.pWaitSemaphores      = &m_present_semaphore;
        submit.pWaitDstStageMask    = &wait_stage;
        submit.commandBufferCount   = 1;
        submit.pCommandBuffers      = &m_command_buffer;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores    = &m_render_semaphore;

        vkQueueSubmit(m_context.graphics_queue(), 1, &submit, m_render_fence);

        VkPresentInfoKHR present{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores    = &m_render_semaphore;
        present.swapchainCount     = 1;
        present.pSwapchains        = &m_swapchain;
        present.pImageIndices      = &m_swapchain_image_index;

        vkQueuePresentKHR(m_context.graphics_queue(), &present);
    }

    void VulkanRenderer::transition_image(VkCommandBuffer cmd, VkImage image,
                                          VkImageLayout from, VkImageLayout to) {
        VkImageMemoryBarrier barrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout        = from;
        barrier.newLayout        = to;
        barrier.image            = image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        barrier.srcAccessMask    = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.dstAccessMask    = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    VulkanRenderer::~VulkanRenderer() {
        VkDevice dev = m_context.device();
        if (dev != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(dev);
            vkDestroySemaphore(dev, m_present_semaphore, nullptr);
            vkDestroySemaphore(dev, m_render_semaphore, nullptr);
            vkDestroyFence(dev, m_render_fence, nullptr);
            vkDestroyCommandPool(dev, m_command_pool, nullptr);
            for (auto view : m_swapchain_image_views)
                vkDestroyImageView(dev, view, nullptr);
            vkDestroySwapchainKHR(dev, m_swapchain, nullptr);
        }

        // m_context destructor handles device, surface, messenger, instance
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    bool VulkanRenderer::should_close() const {
        return glfwWindowShouldClose(m_window);
    }

    void VulkanRenderer::poll_events() {
        glfwPollEvents();
    }
    void VulkanRenderer::record_draw(const ArenaSlice& slice) {
        // 1. BIND PIPELINE FIRST
        // This tells Vulkan which "program" and "layout" we are using
        vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_line_pipeline.pipeline());

        // 2. SET DYNAMIC STATE
        VkViewport viewport{0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 1.0f};
        vkCmdSetViewport(m_command_buffer, 0, 1, &viewport);

        VkRect2D scissor{{0, 0}, {m_width, m_height}};
        vkCmdSetScissor(m_command_buffer, 0, 1, &scissor);

        // 3. PUSH CONSTANTS
        // Now that the pipeline is bound, we push the data to its layout
        struct PushConstants {
            glm::mat4 mvp;
            int mode;
            float color[3];
        } constants;

        constants.mvp = glm::mat4(1.0f);
        constants.mode = 0;
        // (Optional) constants.color initialization if mode != 0

        vkCmdPushConstants(
            m_command_buffer,
            m_line_pipeline.layout(), // CRITICAL: This must match the layout used to create the pipeline
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(PushConstants),
            &constants
        );

        // 4. BIND VERTICES & DRAW
        VkDeviceSize offset = slice.offset;
        vkCmdBindVertexBuffers(m_command_buffer, 0, 1, &slice.buffer, &offset);

        vkCmdDraw(m_command_buffer, slice.vertexCount, 1, 0, 0);
    }

    void VulkanRenderer::init_pipelines() {
        // 1. Get the device from the context
        VkDevice device = m_context.device();

        // 2. Get the format from the context
        // (Ensure m_context has a swapchain_format() getter!)
        VkFormat format = m_context.swapchain_format();

        m_line_pipeline.init(
            device,
            format,
            SHADER_DIR "/curve.vert.spv",
            SHADER_DIR "/curve.frag.spv",
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        );
    }

} // namespace ndde