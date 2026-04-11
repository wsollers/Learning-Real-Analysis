#pragma once
// renderer/ImGuiLayer.hpp
// Thin wrapper around Dear ImGui's GLFW + Vulkan backend.
// Owns the descriptor pool ImGui requires; everything else is handled
// by imgui_impl_glfw and imgui_impl_vulkan.

#define GLFW_INCLUDE_VULKAN
#include <string>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "imgui.h"

namespace ndde::renderer {

class VulkanContext;
class Swapchain;

class ImGuiLayer {
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    void init(GLFWwindow* window,
              const VulkanContext& ctx,
              const Swapchain& swapchain,
              const std::string& assets_dir);

    ImFont* font_math_body()  const { return m_font_math_body; }
    ImFont* font_math_small() const { return m_font_math_small; }

    void destroy();

    /// Begin a new ImGui frame. Call once per frame before building UI.
    void new_frame();

    /// Record ImGui draw commands into cmd.
    /// Must be called inside an active vkCmdBeginRendering block.
    void render(VkCommandBuffer cmd);

    /// Call after swapchain recreation so ImGui knows the new image count / format.
    void on_resize(const Swapchain& swapchain);

private:
    VkDescriptorPool m_pool        = VK_NULL_HANDLE;
    VkDevice         m_device      = VK_NULL_HANDLE;
    bool             m_initialised = false;

    ImFont* m_font_math_body  = nullptr;
    ImFont* m_font_math_small = nullptr;

};

} // namespace ndde::renderer
