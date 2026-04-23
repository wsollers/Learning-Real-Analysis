// engine/Engine.cpp
#define VOLK_IMPLEMENTATION
#include <volk.h>

#include "engine/Engine.hpp"
#include "app/Scene.hpp"
#include "app/SurfaceSimScene.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <format>
#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace ndde {

Engine::Engine() = default;

Engine::~Engine() {
    m_sim_scene.reset();
    m_scene.reset();
    m_second_win.destroy();
    m_renderer.destroy();
    m_buffer_manager.destroy();
    m_swapchain.destroy();
    m_vk.destroy();
    m_glfw.destroy();
}

void Engine::start(const std::string& config_path) {
    std::cout << "[Engine] Starting...\n";

    m_config = AppConfig::load_or_default(config_path);

    m_glfw.init(m_config.window.width,
                m_config.window.height,
                m_config.window.title);

    if (volkInitialize() != VK_SUCCESS)
        throw std::runtime_error("[Engine] volkInitialize() failed");

    m_vk.init(m_glfw.window(), m_config.window.title);
    m_swapchain.init(m_vk, m_glfw.width(), m_glfw.height());
    m_renderer.init(m_vk, m_swapchain, SHADER_DIR, ASSETS_DIR, m_glfw.window());

    m_buffer_manager.init(m_vk.device(), m_vk.physical_device(),
                          m_config.simulation.arena_size_mb);

    // Second window for the 2D contour view.
    // Place it to the right of the primary window. glfwGetMonitors lets us
    // find the second monitor; fall back to primary + offset if only one.
    {
        int   mon_count = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&mon_count);
        int x = 0, y = 0;
        if (mon_count >= 2) {
            // Put second window on second monitor
            int mx=0, my=0;
            glfwGetMonitorPos(monitors[1], &mx, &my);
            const GLFWvidmode* vm = glfwGetVideoMode(monitors[1]);
            x = mx; y = my;
            m_second_win.init(m_vk, x, y,
                static_cast<u32>(vm->width),
                static_cast<u32>(vm->height),
                "Contour 2D", SHADER_DIR);
        } else {
            // Single monitor: place second window at right half
            int mx=0, my=0;
            glfwGetMonitorPos(monitors[0], &mx, &my);
            const GLFWvidmode* vm = glfwGetVideoMode(monitors[0]);
            const u32 hw = static_cast<u32>(vm->width / 2);
            m_second_win.init(m_vk, mx + static_cast<int>(hw), my,
                hw, static_cast<u32>(vm->height),
                "Contour 2D", SHADER_DIR);
        }
    }

    // Also maximise the primary window to fill its half / first monitor
    glfwMaximizeWindow(m_glfw.window());

    m_scene     = std::make_unique<Scene>(make_api());
    m_sim_scene = std::make_unique<SurfaceSimScene>(make_api());

    m_last_frame_time = glfwGetTime();
    m_running = true;
    std::cout << "[Engine] Ready.\n";
}

void Engine::run() {
    while (m_running && !m_glfw.should_close()) {
        m_glfw.poll_events();
        if (m_glfw.check_resize()) { handle_resize(); continue; }
        run_frame();
    }
    vkDeviceWaitIdle(m_vk.device());
}

void Engine::run_frame() {
    // ── Wall-clock timing ─────────────────────────────────────────────────────
    const double now      = glfwGetTime();
    const double delta_s  = now - m_last_frame_time;
    m_last_frame_time     = now;
    const f32 frame_ms    = static_cast<f32>(delta_s * 1000.0);
    const f32 fps         = (frame_ms > 0.f) ? 1000.f / frame_ms : 0.f;

    m_buffer_manager.reset();

    // ── Primary window (3D surface + ImGui) ─────────────────────────────────
    if (!m_renderer.begin_frame(m_swapchain)) { handle_resize(); return; }
    m_renderer.imgui_new_frame();

    // ── Populate DebugStats ───────────────────────────────────────────────────
    const auto& sc_ext = m_swapchain.extent();
    m_debug_stats = DebugStats{
        .arena_bytes_used   = m_buffer_manager.bytes_used(),   // 0 after reset — updated lazily
        .arena_bytes_total  = m_buffer_manager.bytes_total(),
        .arena_utilisation  = 0.f,                             // updated after scene runs
        .arena_vertex_count = 0,
        .draw_calls         = m_renderer.draw_call_count(),    // previous frame
        .swapchain_w        = sc_ext.width,
        .swapchain_h        = sc_ext.height,
        .frame_ms           = frame_ms,
        .fps                = fps,
    };

    // Run only the surface simulation scene.
    // m_scene->on_frame();  // original conics scene — re-enable to switch

    // ── Second window begin ──────────────────────────────────────────────────────
    const bool second_ok = m_second_win.valid() && m_second_win.begin_frame();

    m_sim_scene->on_frame(frame_ms / 1000.f);

    // ── Second window end ─────────────────────────────────────────────────────
    if (second_ok) {
        const bool present_ok = m_second_win.end_frame();
        if (!present_ok) { /* resize handled internally */ }
    }

    // ── Update arena stats after scene geometry has been written ─────────────
    m_debug_stats.arena_bytes_used   = m_buffer_manager.bytes_used();
    m_debug_stats.arena_utilisation  = m_buffer_manager.utilisation();
    m_debug_stats.arena_vertex_count =
        m_buffer_manager.bytes_used() / static_cast<u64>(sizeof(Vertex));

    m_renderer.imgui_render();
    if (!m_renderer.end_frame(m_swapchain)) handle_resize();
}

void Engine::handle_resize() {
    const u32 w = m_glfw.width();
    const u32 h = m_glfw.height();
    if (w == 0 || h == 0) return;
    vkDeviceWaitIdle(m_vk.device());
    m_swapchain.recreate(m_vk, w, h);
    m_renderer.on_swapchain_recreated(m_swapchain);
    std::cout << std::format("[Engine] Swapchain {}x{}\n", w, h);
}

EngineAPI Engine::make_api() {
    EngineAPI api;

    api.acquire = [this](u32 n) -> memory::ArenaSlice {
        return m_buffer_manager.acquire(n);
    };

    api.submit = [this](const memory::ArenaSlice& slice,
                        Topology topology, DrawMode mode, Vec4 color, Mat4 mvp) {
        m_renderer.draw(renderer::DrawCall{
            .slice = slice, .topology = topology,
            .mode  = mode,  .color    = color, .mvp = mvp
        });
    };

    api.submit2 = [this](const memory::ArenaSlice& slice,
                         Topology topology, DrawMode mode, Vec4 color, Mat4 mvp) {
        if (m_second_win.valid())
            m_second_win.draw(renderer::DrawCall{
                .slice = slice, .topology = topology,
                .mode  = mode,  .color    = color, .mvp = mvp
            });
    };

    api.math_font_body  = [this]() -> ImFont* { return m_renderer.font_math_body();  };
    api.math_font_small = [this]() -> ImFont* { return m_renderer.font_math_small(); };
    api.config          = [this]() -> const AppConfig& { return m_config; };
    api.viewport_size   = [this]() -> Vec2 {
        return Vec2{ static_cast<f32>(m_glfw.width()),
                     static_cast<f32>(m_glfw.height()) };
    };
    api.viewport_size2  = [this]() -> Vec2 {
        if (!m_second_win.valid()) return {};
        return Vec2{ static_cast<f32>(m_second_win.width()),
                     static_cast<f32>(m_second_win.height()) };
    };
    api.debug_stats     = [this]() -> const DebugStats& { return m_debug_stats; };

    return api;
}

} // namespace ndde
