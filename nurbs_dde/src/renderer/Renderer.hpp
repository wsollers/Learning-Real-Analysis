#pragma once
// renderer/Renderer.hpp
// Three-phase frame API:
//   begin_frame() → [ImGui calls + draw_scene()] → end_frame()

#include "VulkanContext.hpp"
#include "Swapchain.hpp"
#include "Pipeline.hpp"
#include "ImGuiLayer.hpp"
#include "math/Vec3.hpp"
#include <span>
#include <vector>
#include <cstdint>
#include <string>

namespace ndde::renderer {

struct FrameData {
    VkCommandPool   cmd_pool    = VK_NULL_HANDLE;
    VkCommandBuffer cmd_buf     = VK_NULL_HANDLE;
    VkSemaphore     img_avail   = VK_NULL_HANDLE;
    VkSemaphore     render_done = VK_NULL_HANDLE;
    VkFence         in_flight   = VK_NULL_HANDLE;
};

static constexpr std::uint32_t FRAMES_IN_FLIGHT = 2;

struct GpuBuffer {
    VkBuffer       buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize   size   = 0;
};

struct CurveBuffer {
    GpuBuffer     buf;
    std::uint32_t total_count   = 0;
    std::uint32_t visible_count = 0;
    float         colour[3]     = {1.f, 1.f, 1.f};
    bool          is_hyperbola  = false;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init(GLFWwindow* window,
              const VulkanContext& ctx,
              Swapchain& swapchain,
              const std::string& vert_spv,
              const std::string& frag_spv,
              const std::string& assets_dir);

    ImFont* font_math_body()  const { return m_imgui.font_math_body(); }
    ImFont* font_math_small() const { return m_imgui.font_math_small(); }

    void destroy();

    void on_resize(const VulkanContext& ctx,
                   Swapchain& swapchain,
                   int width, int height);

    // ── Curve slots ───────────────────────────────────────────────────────────
    void upload_curve(const VulkanContext& ctx,
                      std::size_t idx,
                      std::span<const math::Vec4> verts,
                      const float colour[3],
                      bool is_hyperbola = false);

    void set_curve_fraction(std::size_t idx, float fraction);
    void remove_curve(const VulkanContext& ctx, std::size_t idx);

    // ── Axes ──────────────────────────────────────────────────────────────────
    void upload_axes(const VulkanContext& ctx,
                     std::span<const math::Vec4> verts);

    // ── Epsilon ball ──────────────────────────────────────────────────────────
    void upload_epsilon_ball(const VulkanContext& ctx,
                             std::span<const math::Vec4> verts);
    void clear_epsilon_ball() { m_epsilon_count = 0; }

    // ── Interval lines ────────────────────────────────────────────────────────
    void upload_interval_lines(const VulkanContext& ctx,
                               std::span<const math::Vec4> verts);
    void clear_interval_lines() { m_interval_count = 0; }

    void upload_centre_lines(const VulkanContext& ctx,
                             std::span<const math::Vec4> verts);
    void clear_centre_lines() { m_centre_count = 0; }

    // ── Secant line ───────────────────────────────────────────────────────────
    void upload_secant_line(const VulkanContext& ctx,
                            std::span<const math::Vec4> verts,
                            const float colour[3]);
    void clear_secant_line() { m_secant_count = 0; }

    // ── Three-phase frame API ─────────────────────────────────────────────────
    bool begin_frame(const VulkanContext& ctx, Swapchain& swapchain);
    void draw_scene(const math::Mat4& mvp);
    bool end_frame(const VulkanContext& ctx, Swapchain& swapchain);

private:
    Pipeline   m_curve_pipeline;
    Pipeline   m_hyperbola_pipeline;
    Pipeline   m_axes_pipeline;
    Pipeline   m_ball_pipeline;
    ImGuiLayer m_imgui;

    FrameData     m_frames[FRAMES_IN_FLIGHT];
    std::uint32_t m_current_frame = 0;

    VkCommandBuffer m_active_cmd   = VK_NULL_HANDLE;
    std::uint32_t   m_active_img   = 0;
    VkImage         m_active_image = VK_NULL_HANDLE;

    std::vector<CurveBuffer> m_curves;

    GpuBuffer     m_axes_buf;
    std::uint32_t m_axes_count     = 0;

    GpuBuffer     m_epsilon_buf;
    std::uint32_t m_epsilon_count  = 0;

    GpuBuffer     m_interval_buf;
    std::uint32_t m_interval_count = 0;

    GpuBuffer     m_centre_buf;
    std::uint32_t m_centre_count   = 0;

    GpuBuffer     m_secant_buf;
    std::uint32_t m_secant_count   = 0;
    float         m_secant_colour[3] = {1.f, 1.f, 0.f};

    VkDevice m_device = VK_NULL_HANDLE;

    void create_frames(VkDevice device, std::uint32_t queue_family);
    void destroy_frames(VkDevice device);

    void upload_to_buffer(const VulkanContext& ctx,
                          GpuBuffer& buf,
                          std::span<const math::Vec4> verts,
                          std::uint32_t& count);

    void draw_buffer(VkCommandBuffer cmd,
                     VkPipeline pipeline,
                     VkPipelineLayout layout,
                     const math::Mat4& mvp,
                     int mode,
                     const float colour[3],
                     VkBuffer buf,
                     std::uint32_t count);

    GpuBuffer     create_buffer(const VulkanContext& ctx,
                                 VkDeviceSize size,
                                 VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags props);
    void          destroy_buffer(VkDevice device, GpuBuffer& buf);
    std::uint32_t find_memory_type(VkPhysicalDevice phys,
                                    std::uint32_t type_filter,
                                    VkMemoryPropertyFlags props);
    void          transition_image(VkCommandBuffer cmd,
                                    VkImage image,
                                    VkImageLayout from,
                                    VkImageLayout to);
};

} // namespace ndde::renderer