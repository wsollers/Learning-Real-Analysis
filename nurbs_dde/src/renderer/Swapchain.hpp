#pragma once
// renderer/Swapchain.hpp
// Manages the Vulkan swapchain using vk-bootstrap.
#include <volk.h>
#include "VulkanContext.hpp"
#include <VkBootstrap.h>
#include <vector>
#include <cstdint>

namespace ndde::renderer {

class Swapchain {
public:
    Swapchain() = default;
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    void init(const VulkanContext& ctx, int width, int height);
    void recreate(const VulkanContext& ctx, int width, int height);
    void destroy(VkDevice device);

    [[nodiscard]] VkSwapchainKHR           swapchain()   const { return m_swapchain; }
    [[nodiscard]] VkFormat                 format()      const { return m_format; }
    [[nodiscard]] VkExtent2D               extent()      const { return m_extent; }
    [[nodiscard]] const std::vector<VkImage>&     images()     const { return m_images; }
    [[nodiscard]] const std::vector<VkImageView>& image_views() const { return m_image_views; }
    [[nodiscard]] std::uint32_t            image_count() const {
        return static_cast<std::uint32_t>(m_images.size());
    }

private:
    VkDevice         m_device    = VK_NULL_HANDLE;
    VkSwapchainKHR   m_swapchain = VK_NULL_HANDLE;
    VkFormat         m_format    = VK_FORMAT_UNDEFINED;
    VkExtent2D       m_extent    = {};

    std::vector<VkImage>     m_images;
    std::vector<VkImageView> m_image_views;

    void create_image_views(VkDevice device);
    void destroy_image_views(VkDevice device);
};

} // namespace ndde::renderer
