#include "renderer/VulkanRenderer.hpp"
#include "app/AppConfig.hpp"
#include <VkBootstrap.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

namespace ndde {

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

} // namespace ndde