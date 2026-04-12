#pragma once
// renderer/VulkanContext.hpp
// Owns instance, device, queues — built with vk-bootstrap.

// DO NOT use GLFW_INCLUDE_VULKAN — volk owns the Vulkan headers
#include <volk.h>
#include <GLFW/glfw3.h>   // after volk, without GLFW_INCLUDE_VULKAN
#include <VkBootstrap.h>
#include <cstdint>

namespace ndde::renderer {

struct QueueFamilies {
    std::uint32_t graphics{};
    std::uint32_t present{};
};

class VulkanContext {
public:
    VulkanContext() = default;
    ~VulkanContext();

    // Non-copyable, movable
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) noexcept;
    VulkanContext& operator=(VulkanContext&&) noexcept;

    void init(GLFWwindow* window);
    void destroy();

    // ── Accessors ──────────────────────────────────────────────────────────────
    [[nodiscard]] VkInstance         instance()        const { return m_instance; }
    [[nodiscard]] VkPhysicalDevice   physical_device() const { return m_physical_device; }
    [[nodiscard]] VkDevice           device()          const { return m_device; }
    [[nodiscard]] VkSurfaceKHR       surface()         const { return m_surface; }
    [[nodiscard]] VkQueue            graphics_queue()  const { return m_graphics_queue; }
    [[nodiscard]] VkQueue            present_queue()   const { return m_present_queue; }
    [[nodiscard]] QueueFamilies      queue_families()  const { return m_queue_families; }
    [[nodiscard]] vkb::Device        vkb_device()      const { return m_vkb_device; }
    [[nodiscard]] VkFormat swapchain_format() const { return m_swapchain_format; }
private:
    VkInstance       m_instance        = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;
    VkSurfaceKHR     m_surface         = VK_NULL_HANDLE;
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    VkDevice         m_device          = VK_NULL_HANDLE;
    VkQueue          m_graphics_queue  = VK_NULL_HANDLE;
    VkQueue          m_present_queue   = VK_NULL_HANDLE;
    QueueFamilies    m_queue_families  = {};
    vkb::Device      m_vkb_device      = {};
    VkFormat m_swapchain_format = VK_FORMAT_B8G8R8A8_SRGB;
    bool m_initialised = false;
};

} // namespace ndde::renderer
