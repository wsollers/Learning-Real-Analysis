// renderer/ImGuiLayer.cpp
#include "renderer/ImGuiLayer.hpp"
#include "renderer/Swapchain.hpp"
#include "platform/VulkanContext.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>

namespace ndde::renderer {

ImGuiLayer::~ImGuiLayer() { destroy(); }

void ImGuiLayer::init(GLFWwindow* window,
                      const platform::VulkanContext& ctx,
                      const Swapchain& swapchain,
                      const std::string& assets_dir)
{
    m_device = ctx.device();

    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32 }
    };
    VkDescriptorPoolCreateInfo pool_info{
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = 32,
        .poolSizeCount = 1,
        .pPoolSizes    = pool_sizes
    };
    if (vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_pool) != VK_SUCCESS)
        throw std::runtime_error("[ImGuiLayer] vkCreateDescriptorPool failed");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // NOTE: ViewportsEnable is intentionally NOT set.
    // With viewports, MousePos is in absolute screen coords which breaks
    // our pixel_to_world conversion (it expects window-relative coords).
    // We don't need floating ImGui windows outside the main window.
    ImGui::StyleColorsDark();

    ImGuiStyle& style  = ImGui::GetStyle();
    style.WindowRounding   = 4.f;
    style.FrameRounding    = 3.f;
    style.GrabRounding     = 3.f;
    style.WindowBorderSize = 1.f;
    style.FramePadding     = ImVec2(6.f, 4.f);

    ImGui_ImplGlfw_InitForVulkan(window, true);

    VkFormat fmt = swapchain.format();
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance       = ctx.instance();
    init_info.PhysicalDevice = ctx.physical_device();
    init_info.Device         = ctx.device();
    init_info.QueueFamily    = ctx.queue_families().graphics;
    init_info.Queue          = ctx.graphics_queue();
    init_info.DescriptorPool = m_pool;
    init_info.MinImageCount  = 2;
    init_info.ImageCount     = swapchain.image_count();
    init_info.UseDynamicRendering = true;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &fmt
    };

    ImGui_ImplVulkan_Init(&init_info);
    load_fonts(assets_dir);
    m_initialised = true;
    std::cout << "[ImGuiLayer] Initialised\n";
}

void ImGuiLayer::load_fonts(const std::string& assets_dir) {
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar math_ranges[] = {
        0x0020, 0x00FF, 0x0370, 0x03FF, 0x2100, 0x214F, 0x2200, 0x22FF, 0
    };
    ImFontConfig cfg;
    cfg.OversampleH = 3;
    cfg.OversampleV = 2;
    const std::string ttf = assets_dir + "/fonts/STIXTwoText-Italic-VariableFont_wght.ttf";
    m_font_math_body  = io.Fonts->AddFontFromFileTTF(ttf.c_str(), 22.f, &cfg, math_ranges);
    m_font_math_small = io.Fonts->AddFontFromFileTTF(ttf.c_str(), 15.f, &cfg, math_ranges);
    if (!m_font_math_body || !m_font_math_small)
        std::cerr << "[ImGuiLayer] Warning: font not found at " << ttf << "\n";
}

void ImGuiLayer::destroy() {
    if (!m_initialised) return;
    vkDeviceWaitIdle(m_device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(m_device, m_pool, nullptr);
    m_pool        = VK_NULL_HANDLE;
    m_device      = VK_NULL_HANDLE;
    m_initialised = false;
}

void ImGuiLayer::new_frame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::render(VkCommandBuffer cmd) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    // No UpdatePlatformWindows — viewports are disabled
}

void ImGuiLayer::on_swapchain_recreated(const Swapchain&) {}

} // namespace ndde::renderer
