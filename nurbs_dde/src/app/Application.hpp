#pragma once
// app/Application.hpp

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "renderer/VulkanContext.hpp"
#include "renderer/Swapchain.hpp"
#include "renderer/Renderer.hpp"
#include "app/CurveEntry.hpp"
#include "app/AnalysisPanel.hpp"
#include "math/Vec3.hpp"

#include <vector>
#include <memory>

namespace ndde::app {

class Application {
public:
    Application();
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    void run();

private:
    // ── Window ─────────────────────────────────────────────────────────────────
    GLFWwindow* m_window  = nullptr;
    int         m_width   = 0;
    int         m_height  = 0;
    bool        m_resized = false;

    // ── Vulkan ─────────────────────────────────────────────────────────────────
    renderer::VulkanContext m_ctx;
    renderer::Swapchain     m_swapchain;
    renderer::Renderer      m_renderer;

    // ── Curve registry ─────────────────────────────────────────────────────────
    std::vector<CurveEntry> m_curves;

    // ── Simulation ─────────────────────────────────────────────────────────────
    float m_sim_speed = 0.25f;

    // ── Viewport ───────────────────────────────────────────────────────────────
    float  m_zoom       = 1.4f;
    float  m_pan_x      = 0.f;
    float  m_pan_y      = 0.f;
    bool   m_panning    = false;
    double m_pan_last_x = 0.0;
    double m_pan_last_y = 0.0;

    // ── Analysis panel (owns epsilon, feature toggles, readout) ───────────────
    AnalysisPanel m_analysis_panel;
    HoverResult   m_hover;

    // ── Timing ─────────────────────────────────────────────────────────────────
    double m_last_time = 0.0;

    // ── Helpers ────────────────────────────────────────────────────────────────
    void init_window();
    void init_vulkan();
    void init_curves();
    void main_loop();
    void cleanup();

    // Curve management
    void rebuild_curve(std::size_t idx);
    void rebuild_axes();

    // Viewport
    float      view_left()   const;
    float      view_right()  const;
    float      view_bottom() const;
    float      view_top()    const;
    math::Mat4 build_mvp()   const;

    // Hover
    HoverResult             compute_hover() const;
    std::vector<math::Vec4> build_epsilon_circle(float cx, float cy, float r) const;

    // Tick
    void tick(double now);

    // UI
    void build_ui();
    void build_master_panel();
    void build_curve_window(std::size_t idx);
    // build_epsilon_window() removed — owned by AnalysisPanel
    std::vector<math::Vec4> build_interval_lines() const;
    std::vector<math::Vec4> build_centre_lines()   const;

    // Callbacks
    static void framebuffer_resize_callback(GLFWwindow* w, int, int);
    static void scroll_callback(GLFWwindow* w, double, double yoffset);
    static void mouse_button_callback(GLFWwindow* w, int button, int action, int);
    static void cursor_pos_callback(GLFWwindow* w, double xpos, double ypos);
};

} // namespace ndde::app