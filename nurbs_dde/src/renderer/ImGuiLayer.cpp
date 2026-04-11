// renderer/ImGuiLayer.cpp
#include "ImGuiLayer.hpp"
#include "VulkanContext.hpp"
#include "Swapchain.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>

namespace ndde::renderer {

ImGuiLayer::~ImGuiLayer() { destroy(); }

void ImGuiLayer::init(GLFWwindow* window,
                      const VulkanContext& ctx,
                      const Swapchain& swapchain,
                      const std::string& assets_dir)
{
    m_device = ctx.device();

    // ── Descriptor pool ────────────────────────────────────────────────────────
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 },
    };
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets       = 16;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes    = pool_sizes;

    if (vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_pool) != VK_SUCCESS)
        throw std::runtime_error("ImGuiLayer: failed to create descriptor pool");

    // ── ImGui context ──────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();

    // ── GLFW backend ──────────────────────────────────────────────────────────
    ImGui_ImplGlfw_InitForVulkan(window, /*install_callbacks=*/true);

    // ── Vulkan backend ────────────────────────────────────────────────────────
    VkFormat fmt = swapchain.format();

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance        = ctx.instance();
    init_info.PhysicalDevice  = ctx.physical_device();
    init_info.Device          = ctx.device();
    init_info.QueueFamily     = ctx.queue_families().graphics;
    init_info.Queue           = ctx.graphics_queue();
    init_info.DescriptorPool  = m_pool;
    init_info.MinImageCount   = 2;
    init_info.ImageCount      = swapchain.image_count();
    init_info.UseDynamicRendering = true;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &fmt;

    ImGui_ImplVulkan_Init(&init_info);

    // ── Fonts (after both backends initialised) ────────────────────────────────


    static const ImWchar math_ranges[] = {
        0x0020, 0x00FF,
        0x0370, 0x03FF,
        0x2200, 0x22FF,
        0x2100, 0x214F,
        0
    };

    ImFontConfig cfg;
    cfg.OversampleH = 3;
    cfg.OversampleV = 2;

    const std::string math_ttf = assets_dir + "/fonts/STIXTwoText-Italic-VariableFont_wght.ttf";
    fprintf(stderr, "[ImGui] Loading math font from: %s\n", math_ttf.c_str());

    m_font_math_body = io.Fonts->AddFontFromFileTTF(
        math_ttf.c_str(), 22.f, &cfg, math_ranges);

    m_font_math_small = io.Fonts->AddFontFromFileTTF(
        math_ttf.c_str(), 15.f, &cfg, math_ranges);

    if (!m_font_math_body || !m_font_math_small)
        fprintf(stderr, "[ImGui] Warning: math font failed to load\n");

    m_initialised = true;
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

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void ImGuiLayer::on_resize(const Swapchain& /*swapchain*/) {
    // SetMinImageCount unsupported in docking branch with dynamic rendering
}

} // namespace ndde::renderer