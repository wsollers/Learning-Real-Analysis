#pragma once
// renderer/Renderer.hpp
// Owns the Vulkan frame loop: command pool, sync objects, pipelines, ImGui.
// Frame order per Engine::run_frame():
//   begin_frame() -> [draw() x N] -> imgui_new_frame/imgui_render -> end_frame()

#include <volk.h>
#include "platform/VulkanContext.hpp"
#include "renderer/Swapchain.hpp"
#include "renderer/Pipeline.hpp"
#include "renderer/ImGuiLayer.hpp"
#include "memory/ArenaSlice.hpp"
#include "math/Types.hpp"
#include <glm/glm.hpp>
#include <string>

struct GLFWwindow;

namespace ndde::renderer {

struct DrawCall {
    memory::ArenaSlice slice;
    Topology           topology = Topology::LineStrip;
    DrawMode           mode     = DrawMode::VertexColor;
    Vec4               color    = { 1.f, 1.f, 1.f, 1.f };
    Mat4               mvp      = glm::mat4(1.f);
};

class Renderer {
public:
    Renderer()  = default;
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init(const platform::VulkanContext& ctx,
              const Swapchain&               swapchain,
              const std::string&             shader_dir,
              const std::string&             assets_dir,
              GLFWwindow*                    window);

    void destroy();

    [[nodiscard]] bool begin_frame(const Swapchain& swapchain);
    void draw(const DrawCall& dc);
    void imgui_new_frame() { m_imgui.new_frame(); }
    void imgui_render()    { m_imgui.render(m_cmd); }
    [[nodiscard]] bool end_frame(const Swapchain& swapchain);
    void on_swapchain_recreated(const Swapchain& swapchain);

    [[nodiscard]] ImFont* font_math_body()  const noexcept { return m_imgui.font_math_body();  }
    [[nodiscard]] ImFont* font_math_small() const noexcept { return m_imgui.font_math_small(); }

private:
    VkDevice        m_device         = VK_NULL_HANDLE;
    VkQueue         m_graphics_queue = VK_NULL_HANDLE;
    VkQueue         m_present_queue  = VK_NULL_HANDLE;
    VkCommandPool   m_cmd_pool       = VK_NULL_HANDLE;
    VkCommandBuffer m_cmd            = VK_NULL_HANDLE;
    VkFence         m_render_fence   = VK_NULL_HANDLE;
    VkSemaphore     m_image_available = VK_NULL_HANDLE;
    VkSemaphore     m_render_finished = VK_NULL_HANDLE;
    u32             m_image_index    = 0;
    bool            m_frame_open     = false;

    Pipeline   m_pipeline_line_list;
    Pipeline   m_pipeline_line_strip;
    Pipeline   m_pipeline_triangle_list;
    ImGuiLayer m_imgui;

    void create_command_objects(u32 graphics_queue_family);
    void create_sync_objects();
    void init_pipelines(VkFormat color_format, const std::string& shader_dir);
    void transition_image(VkImage image, VkImageLayout from, VkImageLayout to);
    [[nodiscard]] Pipeline& pipeline_for(Topology topo);
};

} // namespace ndde::renderer
