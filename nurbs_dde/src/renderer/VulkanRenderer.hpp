#pragma once
#ifndef NDDE_VULKAN_RENDERER_HPP
#define NDDE_VULKAN_RENDERER_HPP

#include "core/Types.hpp"
#include "renderer/VulkanContext.hpp"
#include <vector>

struct GLFWwindow;

namespace ndde {

    struct AppConfig;

    class VulkanRenderer {
    public:
        explicit VulkanRenderer(const AppConfig& config);
        ~VulkanRenderer();

        bool should_close() const;
        void poll_events();
        void begin_frame();
        void end_frame();
        void draw_geometry(const AppConfig& config);

        VkDevice         get_device()          const { return m_context.device(); }
        VkPhysicalDevice get_physical_device() const { return m_context.physical_device(); }

    private:
        void init_window();
        void create_sync_objects();
        void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout from, VkImageLayout to);

        GLFWwindow* m_window = nullptr;
        u32 m_width  = 1280;
        u32 m_height = 720;

        renderer::VulkanContext m_context;

        VkSwapchainKHR           m_swapchain             = VK_NULL_HANDLE;
        std::vector<VkImage>     m_swapchain_images;
        std::vector<VkImageView> m_swapchain_image_views;

        VkCommandPool   m_command_pool   = VK_NULL_HANDLE;
        VkCommandBuffer m_command_buffer = VK_NULL_HANDLE;

        VkFence     m_render_fence      = VK_NULL_HANDLE;
        VkSemaphore m_present_semaphore = VK_NULL_HANDLE;
        VkSemaphore m_render_semaphore  = VK_NULL_HANDLE;
        u32         m_swapchain_image_index = 0;
    };
}

#endif