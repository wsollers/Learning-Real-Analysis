// renderer/Swapchain.cpp
#include "Swapchain.hpp"
#include <stdexcept>

namespace ndde::renderer {

Swapchain::~Swapchain() {
    if (m_device != VK_NULL_HANDLE) destroy(m_device);
}

void Swapchain::init(const VulkanContext& ctx, int width, int height) {
    m_device = ctx.device();

    vkb::SwapchainBuilder builder{
        ctx.physical_device(),
        ctx.device(),
        ctx.surface()
    };

    auto sc_ret = builder
        .set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)   // vsync, always supported
        .set_desired_extent(static_cast<std::uint32_t>(width),
                            static_cast<std::uint32_t>(height))
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build();

    if (!sc_ret) throw std::runtime_error("Swapchain build: " + sc_ret.error().message());

    vkb::Swapchain vkb_sc = sc_ret.value();
    m_swapchain = vkb_sc.swapchain;
    m_format    = vkb_sc.image_format;
    m_extent    = vkb_sc.extent;
    m_images    = vkb_sc.get_images().value();
    m_image_views = vkb_sc.get_image_views().value();
}

void Swapchain::recreate(const VulkanContext& ctx, int width, int height) {
    destroy(ctx.device());
    init(ctx, width, height);
}

void Swapchain::destroy(VkDevice device) {
    destroy_image_views(device);
    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
    m_device = VK_NULL_HANDLE;
}

void Swapchain::create_image_views(VkDevice /*device*/) {
    // vk-bootstrap already built the image views; nothing to do here.
}

void Swapchain::destroy_image_views(VkDevice device) {
    for (auto& iv : m_image_views) {
        vkDestroyImageView(device, iv, nullptr);
    }
    m_image_views.clear();
}

} // namespace ndde::renderer
