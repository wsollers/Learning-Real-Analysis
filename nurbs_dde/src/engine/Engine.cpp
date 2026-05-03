// engine/Engine.cpp
#define VOLK_IMPLEMENTATION
#include <volk.h>

#include "engine/Engine.hpp"
#include "app/Scene.hpp"
#include "app/SceneFactories.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <glm/gtc/matrix_transform.hpp>
#include <format>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifndef NDDE_PROJECT_DIR
#define NDDE_PROJECT_DIR "."
#endif

namespace ndde {

namespace {
std::unordered_map<GLFWwindow*, Engine*> g_hotkey_engines;

} // namespace

void engine_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (auto it = g_hotkey_engines.find(window); it != g_hotkey_engines.end())
        it->second->on_key_event(key, action, mods);
}

Engine::Engine() = default;

Engine::~Engine() {
    uninstall_global_hotkeys();
    m_active.reset();
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
    m_swapchain.init(m_vk, m_glfw.width(), m_glfw.height(), m_config.render.vsync);
    m_renderer.init(m_vk, m_swapchain, SHADER_DIR, ASSETS_DIR, m_glfw.window());
    install_global_hotkeys();

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
                "Contour 2D", SHADER_DIR, m_config.render.vsync);
        } else {
            // Single monitor: place second window at right half
            int mx=0, my=0;
            glfwGetMonitorPos(monitors[0], &mx, &my);
            const GLFWvidmode* vm = glfwGetVideoMode(monitors[0]);
            const u32 hw = static_cast<u32>(vm->width / 2);
            m_second_win.init(m_vk, mx + static_cast<int>(hw), my,
                hw, static_cast<u32>(vm->height),
                "Contour 2D", SHADER_DIR, m_config.render.vsync);
        }
    }

    // Also maximise the primary window to fill its half / first monitor
    glfwMaximizeWindow(m_glfw.window());

    m_simulations.add(std::make_unique<SceneSimulationRuntime>(
        "Surface Simulation", make_surface_sim_scene));
    m_simulations.add(std::make_unique<SceneSimulationRuntime>(
        "Sine-Rational Analysis", make_analysis_scene));
    m_simulations.add(std::make_unique<SceneSimulationRuntime>(
        "Multi-Well Centroid", make_multiwell_scene));

    for (std::size_t i = 0; i < m_simulations.size(); ++i)
        if (auto* sim = m_simulations.get(i))
            sim->instantiate(make_api());

    m_active_sim = 0;
    active_runtime().start();

    m_last_frame_time = glfwGetTime();
    m_running = true;
    std::cout << "[Engine] Ready.\n";
}

void Engine::install_global_hotkeys() {
    GLFWwindow* window = m_glfw.window();
    if (!window) return;
    g_hotkey_engines[window] = this;
    glfwSetKeyCallback(window, engine_key_callback);
#if defined(_WIN32)
    std::cout << "[Engine] Global hotkeys: GLFW event callback (Win32 backend)\n";
#elif defined(__linux__)
    std::cout << "[Engine] Global hotkeys: GLFW event callback (Linux backend)\n";
#else
    std::cout << "[Engine] Global hotkeys: GLFW event callback\n";
#endif
}

void Engine::uninstall_global_hotkeys() noexcept {
    GLFWwindow* window = m_glfw.window();
    if (!window) return;
    g_hotkey_engines.erase(window);
    glfwSetKeyCallback(window, nullptr);
}

void Engine::switch_scene(SceneFactory factory) {
    vkDeviceWaitIdle(m_vk.device());
    // Reset renderer per-frame sync state so the new scene's first frame
    // acquires with clean, unsignaled semaphores and a signaled fence.
    m_renderer.reset_frame_state();
    auto next = factory(make_api());
    m_active  = std::move(next);
}

void Engine::switch_simulation(std::size_t index) {
    if (index >= m_simulations.size() || index == m_active_sim) return;
    vkDeviceWaitIdle(m_vk.device());
    m_renderer.reset_frame_state();
    m_active_sim = index;
    active_runtime().start();
    std::cout << std::format("[Engine] Active simulation: {}\n", active_runtime().name());
}

void Engine::run() {
    while (m_running && !m_glfw.should_close()) {
        m_glfw.poll_events();
        if (m_second_win.should_close()) {
            vkDeviceWaitIdle(m_vk.device());
            m_second_win.destroy();
        }
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
    dispatch_global_hotkeys();

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

    active_runtime().scene().on_frame(frame_ms / 1000.f);
    draw_global_panels();

    // ── Update arena stats after scene geometry has been written ─────────────
    m_debug_stats.arena_bytes_used   = m_buffer_manager.bytes_used();
    m_debug_stats.arena_utilisation  = m_buffer_manager.utilisation();
    m_debug_stats.arena_vertex_count =
        m_buffer_manager.bytes_used() / static_cast<u64>(sizeof(Vertex));

    m_renderer.imgui_render();
    const bool primary_ok = m_renderer.end_frame(m_swapchain);

    // Present the auxiliary contour window after the primary window. FIFO
    // present can block, and letting the secondary surface block first makes
    // main-window frame pacing visibly worse on some drivers.
    if (second_ok) {
        const bool present_ok = m_second_win.end_frame();
        if (!present_ok) { /* resize handled internally */ }
    }

    if (!primary_ok) handle_resize();

    apply_pending_scene_switch();
    apply_pending_simulation_switch();
}

void Engine::draw_global_panels() {
    ImGui::SetNextWindowPos(ImVec2(12.f, 34.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260.f, 150.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.86f);
    if (!ImGui::Begin("Engine - Global")) { ImGui::End(); return; }

    ImGui::SeparatorText("Simulations");
    for (std::size_t i = 0; i < m_simulations.size(); ++i) {
        auto* sim = m_simulations.get(i);
        if (!sim) continue;
        const std::string label = std::format("Ctrl+{}  {}", i + 1, sim->name());
        if (ImGui::Selectable(label.c_str(), i == m_active_sim))
            m_pending_sim = i;
    }

    ImGui::SeparatorText("Stats");
    ImGui::TextDisabled("%.1f ms   %.0f fps", m_debug_stats.frame_ms, m_debug_stats.fps);
    ImGui::TextDisabled("%llu verts   %llu / %llu bytes",
        static_cast<unsigned long long>(m_debug_stats.arena_vertex_count),
        static_cast<unsigned long long>(m_debug_stats.arena_bytes_used),
        static_cast<unsigned long long>(m_debug_stats.arena_bytes_total));
    ImGui::TextDisabled("F12 capture   Ctrl+Shift+P pause+capture");
    ImGui::End();
}

void Engine::apply_pending_scene_switch() {
    if (!m_pending_scene_switch) return;
    auto factory = std::move(m_pending_scene_switch);
    m_pending_scene_switch = {};
    switch_scene(std::move(factory));
}

void Engine::apply_pending_simulation_switch() {
    if (m_pending_sim == static_cast<std::size_t>(-1)) return;
    const std::size_t next = m_pending_sim;
    m_pending_sim = static_cast<std::size_t>(-1);
    switch_simulation(next);
}

void Engine::on_key_event(int key, int action, int mods) {
    if (action != GLFW_PRESS) return;

    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) return;

    const bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    const bool shift = (mods & GLFW_MOD_SHIFT) != 0;

    if (ctrl && !shift && key == GLFW_KEY_1) {
        m_pending_sim = 0;
        return;
    }
    if (ctrl && !shift && key == GLFW_KEY_2) {
        m_pending_sim = 1;
        return;
    }
    if (ctrl && !shift && key == GLFW_KEY_3) {
        m_pending_sim = 2;
        return;
    }
    if (!ctrl && !shift && key == GLFW_KEY_F12) {
        request_capture(false);
        return;
    }
    if (ctrl && shift && key == GLFW_KEY_P) {
        request_capture(true);
        return;
    }

    active_runtime().scene().on_key_event(key, action, mods);
}

void Engine::dispatch_global_hotkeys() {
    // Global hotkeys are delivered by the GLFW key callback installed after
    // ImGui. This function intentionally remains as a frame-loop hook for
    // future event queue draining, but no longer polls key state.
}

void Engine::request_capture(bool pause_first) {
    if (pause_first)
        active_runtime().pause();
    m_renderer.request_png_capture(make_capture_path());
}

std::filesystem::path Engine::make_capture_path() const {
    std::string name = std::string(active_runtime().name());
    for (char& c : name) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (!std::isalnum(uc)) c = '_';
    }
    name.erase(std::unique(name.begin(), name.end(),
                           [](char a, char b){ return a == '_' && b == '_'; }),
               name.end());
    if (!name.empty() && name.back() == '_') name.pop_back();
    if (name.empty()) name = "simulation";

    const std::time_t now = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &now);
    std::ostringstream stamp;
    stamp << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return std::filesystem::path{NDDE_PROJECT_DIR} / "captures" / (name + "_" + stamp.str() + ".png");
}

SceneSimulationRuntime& Engine::active_runtime() {
    auto* runtime = m_simulations.get(m_active_sim);
    if (!runtime) throw std::runtime_error("[Engine] No active simulation runtime");
    return *runtime;
}

const SceneSimulationRuntime& Engine::active_runtime() const {
    const auto* runtime = m_simulations.get(m_active_sim);
    if (!runtime) throw std::runtime_error("[Engine] No active simulation runtime");
    return *runtime;
}

void Engine::handle_resize() {
    const u32 w = m_glfw.width();
    const u32 h = m_glfw.height();
    if (w == 0 || h == 0) return;
    vkDeviceWaitIdle(m_vk.device());
    m_swapchain.recreate(m_vk, w, h, m_config.render.vsync);
    m_renderer.on_swapchain_recreated(m_swapchain);
    std::cout << std::format("[Engine] Swapchain {}x{}\n", w, h);
}

EngineAPI Engine::make_api() {
    EngineAPI api;

    api.acquire = [this](u32 n) -> memory::ArenaSlice {
        return m_buffer_manager.acquire(n);
    };

    api.submit_to = [this](RenderTarget target,
                             const memory::ArenaSlice& slice,
                             Topology topology, DrawMode mode,
                             Vec4 color, Mat4 mvp) {
        renderer::DrawCall dc{
            .slice = slice, .topology = topology,
            .mode  = mode,  .color    = color, .mvp = mvp
        };
        switch (target) {
            case RenderTarget::Primary3D:
                m_renderer.draw(dc);
                break;
            case RenderTarget::Contour2D:
                if (m_second_win.valid()) m_second_win.draw(dc);
                break;
        }
    };

    api.push_math_font = [this](bool small) {
        ImFont* f = small ? m_renderer.font_math_small() : m_renderer.font_math_body();
        if (f) ImGui::PushFont(f);
    };
    api.pop_math_font = []() { ImGui::PopFont(); };
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

    api.switch_scene = [this](SceneFactory factory) {
        m_pending_scene_switch = std::move(factory);
    };

    api.switch_simulation = [this](std::size_t index) {
        m_pending_sim = index;
    };

    return api;
}

} // namespace ndde
