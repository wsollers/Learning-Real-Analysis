// engine/Engine.cpp
#define VOLK_IMPLEMENTATION
#include <volk.h>

#include "engine/Engine.hpp"
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
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifndef NDDE_PROJECT_DIR
#define NDDE_PROJECT_DIR "."
#endif

namespace ndde {

namespace {
std::unordered_map<GLFWwindow*, Engine*> g_hotkey_engines;

std::string_view render_kind_name(RenderViewKind kind) noexcept {
    return kind == RenderViewKind::Main ? "Main" : "Alternate";
}

std::string_view alternate_mode_name(AlternateViewMode mode) noexcept {
    switch (mode) {
        case AlternateViewMode::Contour: return "Contour";
        case AlternateViewMode::LevelCurves: return "Level Curves";
        case AlternateViewMode::VectorField: return "Vector Field";
        case AlternateViewMode::Isoclines: return "Isoclines";
        case AlternateViewMode::Flow: return "Flow";
    }
    return "Unknown";
}

} // namespace

void engine_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (auto it = g_hotkey_engines.find(window); it != g_hotkey_engines.end())
        it->second->on_key_event(key, action, mods);
}

Engine::Engine()
    : m_simulation_host(m_services.simulation_host())
    , m_simulations(m_services.memory()) {}

Engine::~Engine() {
    uninstall_global_hotkeys();
    m_second_win.destroy();
    m_renderer.destroy();
    m_services.memory().destroy();
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

    m_services.memory().init_frame_gpu_arena(m_vk.device(), m_vk.physical_device(),
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

    register_global_panels();
    register_default_simulations(m_simulations);

    m_active_sim = 0;
    active_runtime().instantiate(m_simulation_host);
    active_runtime().start();

    m_last_frame_time = glfwGetTime();
    m_running = true;
    m_event_log.push_back("Engine ready");
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

void Engine::switch_simulation(std::size_t index) {
    if (index >= m_simulations.size() || index == m_active_sim) return;
    vkDeviceWaitIdle(m_vk.device());
    m_renderer.reset_frame_state();
    active_runtime().stop();
    m_services.render().clear_packets();
    m_services.memory().reset_simulation();
    m_services.memory().reset_cache();
    m_services.memory().reset_history();
    m_active_sim = index;
    active_runtime().instantiate(m_simulation_host);
    active_runtime().start();
    m_event_log.push_back(std::format("Switched simulation: {}", active_runtime().name()));
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

    // Destroy previous-frame packet payloads before releasing frame PMR memory.
    m_services.render().clear_packets();
    m_services.memory().begin_frame();

    // ── Primary window (3D surface + ImGui) ─────────────────────────────────
    if (!m_renderer.begin_frame(m_swapchain)) { handle_resize(); return; }
    m_renderer.imgui_new_frame();
    dispatch_global_hotkeys();
    update_render_view_input();

    // ── Populate DebugStats ───────────────────────────────────────────────────
    const auto& sc_ext = m_swapchain.extent();
    m_debug_stats = DebugStats{
        .arena_bytes_used   = m_services.memory().frame_gpu_bytes_used(),   // 0 after reset — updated lazily
        .arena_bytes_total  = m_services.memory().frame_gpu_bytes_total(),
        .arena_utilisation  = 0.f,                             // updated after scene runs
        .arena_vertex_count = 0,
        .draw_calls         = m_renderer.draw_call_count(),    // previous frame
        .swapchain_w        = sc_ext.width,
        .swapchain_h        = sc_ext.height,
        .frame_ms           = frame_ms,
        .fps                = fps,
    };

    // ── Second window begin ──────────────────────────────────────────────────────
    const bool second_ok = m_second_win.valid() && m_second_win.begin_frame();

    active_runtime().tick(m_services.clock().next(frame_ms / 1000.f, active_runtime().paused()));
    flush_render_service();
    m_services.panels().draw_registered_panels();

    // ── Update arena stats after scene geometry has been written ─────────────
    m_debug_stats.arena_bytes_used   = m_services.memory().frame_gpu_bytes_used();
    m_debug_stats.arena_utilisation  = m_services.memory().frame_gpu_utilisation();
    m_debug_stats.arena_vertex_count =
        m_services.memory().frame_gpu_bytes_used() / static_cast<u64>(sizeof(Vertex));

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

    apply_pending_simulation_switch();
}

void Engine::register_global_panels() {
    m_global_panels.clear();
    m_global_panels.push_back(m_services.panels().register_panel(PanelDescriptor{
        .title = "Engine - Global",
        .category = "Engine",
        .scope = PanelScope::Global,
        .draw = [this] { draw_global_status_panel(); }
    }));
    m_global_panels.push_back(m_services.panels().register_panel(PanelDescriptor{
        .title = "Debug - Coordinates",
        .category = "Debug",
        .scope = PanelScope::Global,
        .draw = [this] { draw_debug_coordinates_panel(); }
    }));
    m_global_panels.push_back(m_services.panels().register_panel(PanelDescriptor{
        .title = "Simulation - Metadata",
        .category = "Debug",
        .scope = PanelScope::Global,
        .draw = [this] { draw_simulation_metadata_panel(); }
    }));
    m_global_panels.push_back(m_services.panels().register_panel(PanelDescriptor{
        .title = "Engine - Log",
        .category = "Engine",
        .scope = PanelScope::Global,
        .draw = [this] { draw_event_log_panel(); }
    }));
}

void Engine::draw_global_status_panel() {
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
    const auto sim_snapshot = active_runtime().snapshot();
    ImGui::TextDisabled("%s   %s", sim_snapshot.name.c_str(), sim_snapshot.status.c_str());
    if (sim_snapshot.sim_speed > 0.f || sim_snapshot.particle_count > 0) {
        ImGui::TextDisabled("t %.2f   speed %.2f   %llu particles",
            sim_snapshot.sim_time,
            sim_snapshot.sim_speed,
            static_cast<unsigned long long>(sim_snapshot.particle_count));
    }
    ImGui::TextDisabled("%.1f ms   %.0f fps", m_debug_stats.frame_ms, m_debug_stats.fps);
    ImGui::TextDisabled("%llu verts   %llu / %llu bytes",
        static_cast<unsigned long long>(m_debug_stats.arena_vertex_count),
        static_cast<unsigned long long>(m_debug_stats.arena_bytes_used),
        static_cast<unsigned long long>(m_debug_stats.arena_bytes_total));
    ImGui::TextDisabled("F12 capture   Ctrl+Shift+P pause+capture");
    bool axes = m_services.render().axes_visible();
    if (ImGui::Checkbox("Axes", &axes))
        m_services.render().set_axes_visible(axes);
    ImGui::SeparatorText("Camera");
    if (ImGui::Button("Home")) m_services.camera().reset_main(CameraPreset::Home);
    ImGui::SameLine();
    if (ImGui::Button("Top")) m_services.camera().reset_main(CameraPreset::Top);
    ImGui::SameLine();
    if (ImGui::Button("Front")) m_services.camera().reset_main(CameraPreset::Front);
    ImGui::SameLine();
    if (ImGui::Button("Side")) m_services.camera().reset_main(CameraPreset::Side);
    if (ImGui::Button("Frame Selection"))
        (void)m_services.camera().frame_selection(m_services.interaction());
    ImGui::TextDisabled("RMB drag orbit   MMB/Shift+RMB pan   Wheel zoom");
    ImGui::TextDisabled("Double-click surface perturb");
    ImGui::End();
}

void Engine::draw_debug_coordinates_panel() {
    ImGui::SetNextWindowPos(ImVec2(290.f, 34.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340.f, 220.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.86f);
    if (!ImGui::Begin("Debug - Coordinates")) { ImGui::End(); return; }

    const ImGuiIO& io = ImGui::GetIO();
    const Vec2 fb{static_cast<f32>(m_glfw.width()), static_cast<f32>(m_glfw.height())};
    ImGui::SeparatorText("Mouse");
    ImGui::TextDisabled("display %.0f x %.0f", io.DisplaySize.x, io.DisplaySize.y);
    ImGui::TextDisabled("framebuffer %.0f x %.0f", fb.x, fb.y);
    ImGui::TextDisabled("mouse %.1f, %.1f", io.MousePos.x, io.MousePos.y);

    const RenderViewId main = m_services.render().first_active_main_view();
    if (main != 0) {
        const RenderViewDomain d = m_services.render().view_domain(main);
        const float nx = io.DisplaySize.x > 0.f ? std::clamp(io.MousePos.x / io.DisplaySize.x, 0.f, 1.f) : 0.f;
        const float ny = io.DisplaySize.y > 0.f ? std::clamp(io.MousePos.y / io.DisplaySize.y, 0.f, 1.f) : 0.f;
        const Vec2 uv{
            d.u_min + nx * (d.u_max - d.u_min),
            d.v_max - ny * (d.v_max - d.v_min)
        };
        ImGui::SeparatorText("Active Main View");
        ImGui::TextDisabled("domain u[%.2f, %.2f] v[%.2f, %.2f]", d.u_min, d.u_max, d.v_min, d.v_max);
        ImGui::TextDisabled("mapped uv %.3f, %.3f", uv.x, uv.y);
        if (const auto* desc = m_services.render().descriptor(main)) {
            const auto& cam = desc->camera;
            ImGui::TextDisabled("camera yaw %.2f pitch %.2f zoom %.2f", cam.yaw, cam.pitch, cam.zoom);
            ImGui::TextDisabled("target %.2f, %.2f, %.2f", cam.target.x, cam.target.y, cam.target.z);
            ImGui::TextDisabled("view %.0f x %.0f aspect %.3f",
                desc->viewport_size.x, desc->viewport_size.y, desc->viewport_aspect);
        }
    }
    const HoverMetadata& hover = m_services.interaction().hover_metadata();
    ImGui::SeparatorText("Hover");
    ImGui::TextDisabled("view %llu   mouse %.1f, %.1f",
        static_cast<unsigned long long>(hover.view),
        hover.mouse_pixel.x,
        hover.mouse_pixel.y);
    if (hover.surface.hit) {
        ImGui::TextDisabled("surface uv %.3f, %.3f", hover.surface.uv.x, hover.surface.uv.y);
        ImGui::TextDisabled("world %.3f, %.3f, %.3f",
            hover.surface.world.x, hover.surface.world.y, hover.surface.world.z);
    } else {
        ImGui::TextDisabled("surface: no hit");
    }
    if (hover.particle.hit) {
        ImGui::TextDisabled("particle %llu idx %u  d %.1f px",
            static_cast<unsigned long long>(hover.particle.particle_id),
            hover.particle.trail_index,
            hover.particle.pixel_distance);
        ImGui::TextDisabled("k %.5f   tau %.5f", hover.particle.curvature, hover.particle.torsion);
    } else {
        ImGui::TextDisabled("particle: no trail snap");
    }
    if (hover.view_point.hit) {
        ImGui::TextDisabled("2D point %.3f, %.3f", hover.view_point.point.x, hover.view_point.point.y);
        ImGui::TextDisabled("2D world %.3f, %.3f, %.3f",
            hover.view_point.world.x,
            hover.view_point.world.y,
            hover.view_point.world.z);
    }
    auto kind_name = [](InteractionTargetKind kind) {
        switch (kind) {
            case InteractionTargetKind::SurfacePoint: return "SurfacePoint";
            case InteractionTargetKind::ViewPoint2D: return "ViewPoint2D";
            case InteractionTargetKind::Particle: return "Particle";
            case InteractionTargetKind::TrailSample: return "TrailSample";
            case InteractionTargetKind::None:
            default: return "None";
        }
    };
    const InteractionTarget selected = m_services.interaction().selected_target();
    ImGui::SeparatorText("Selection");
    ImGui::TextDisabled("kind %s   view %llu",
        kind_name(selected.kind),
        static_cast<unsigned long long>(selected.view));
    if (selected.valid) {
        if (selected.kind == InteractionTargetKind::SurfacePoint) {
            ImGui::TextDisabled("uv %.3f, %.3f", selected.uv.x, selected.uv.y);
        }
        if (selected.kind == InteractionTargetKind::ViewPoint2D) {
            ImGui::TextDisabled("point %.3f, %.3f", selected.point2d.x, selected.point2d.y);
        }
        ImGui::TextDisabled("world %.3f, %.3f, %.3f",
            selected.world.x, selected.world.y, selected.world.z);
        if (selected.kind == InteractionTargetKind::TrailSample || selected.kind == InteractionTargetKind::Particle) {
            ImGui::TextDisabled("particle %llu trail %u",
                static_cast<unsigned long long>(selected.particle_id),
                selected.trail_index);
            ImGui::TextDisabled("k %.5f   tau %.5f", selected.curvature, selected.torsion);
        }
    }
    ImGui::End();
}

void Engine::draw_simulation_metadata_panel() {
    ImGui::SetNextWindowPos(ImVec2(650.f, 34.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380.f, 420.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.86f);
    if (!ImGui::Begin("Simulation - Metadata")) { ImGui::End(); return; }

    const SimulationMetadata metadata = active_runtime().metadata();
    const SceneSnapshot snapshot = active_runtime().snapshot();
    ImGui::SeparatorText("Simulation");
    ImGui::TextDisabled("%s", metadata.name.c_str());
    if (!metadata.surface_name.empty())
        ImGui::TextDisabled("%s", metadata.surface_name.c_str());
    if (!metadata.surface_formula.empty())
        ImGui::TextDisabled("%s", metadata.surface_formula.c_str());
    ImGui::TextDisabled("surface: %s derivatives   %s   %s",
        metadata.surface_has_analytic_derivatives ? "analytic" : "finite-diff",
        metadata.surface_deformable ? "deformable" : "static",
        metadata.surface_time_varying ? "time-varying" : "time-invariant");
    ImGui::TextDisabled("status %s   paused %s", metadata.status.c_str(), metadata.paused ? "yes" : "no");
    ImGui::TextDisabled("t %.2f   speed %.2f   particles %llu",
        metadata.sim_time,
        metadata.sim_speed,
        static_cast<unsigned long long>(metadata.particle_count));

    ImGui::SeparatorText("Render Views");
    for (const RenderViewSnapshot& view : m_services.render().active_view_snapshots()) {
        if (ImGui::TreeNode(std::format("{}##view{}", view.title, view.id).c_str())) {
            ImGui::TextDisabled("id %llu   %s", static_cast<unsigned long long>(view.id), render_kind_name(view.kind).data());
            if (view.kind == RenderViewKind::Alternate)
                ImGui::TextDisabled("mode %s", alternate_mode_name(view.alternate_mode).data());
            ImGui::TextDisabled("domain u[%.2f, %.2f] v[%.2f, %.2f]",
                view.domain.u_min, view.domain.u_max, view.domain.v_min, view.domain.v_max);
            ImGui::TextDisabled("axes %s   hover %s   osc %s",
                view.overlays.show_axes ? "on" : "off",
                view.overlays.show_hover_frenet ? "on" : "off",
                view.overlays.show_osculating_circle ? "on" : "off");
            ImGui::TreePop();
        }
    }

    ImGui::SeparatorText("Particles");
    for (const auto& particle : snapshot.particles) {
        ImGui::TextDisabled("#%llu %s", static_cast<unsigned long long>(particle.id), particle.label.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(%.2f, %.2f, %.2f)", particle.x, particle.y, particle.z);
    }
    ImGui::End();
}

void Engine::draw_event_log_panel() {
    ImGui::SetNextWindowPos(ImVec2(1048.f, 34.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320.f, 180.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.86f);
    if (!ImGui::Begin("Engine - Log")) { ImGui::End(); return; }
    for (const auto& line : m_event_log)
        ImGui::TextDisabled("%s", line.c_str());
    ImGui::End();
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

    if (ctrl && !shift && key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
        const std::size_t index = static_cast<std::size_t>(key - GLFW_KEY_1);
        if (index < m_simulations.size())
            m_pending_sim = index;
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

    (void)m_services.hotkeys().dispatch(KeyChord{.key = key, .mods = mods});
}

void Engine::dispatch_global_hotkeys() {
    // Global hotkeys are delivered by the GLFW key callback installed after
    // ImGui. This function intentionally remains as a frame-loop hook for
    // future event queue draining, but no longer polls key state.
}

void Engine::update_render_view_input() {
    ImGuiIO& io = ImGui::GetIO();
    m_services.render().set_viewport_size(RenderViewKind::Main,
        Vec2{static_cast<f32>(m_glfw.width()), static_cast<f32>(m_glfw.height())});
    if (m_second_win.valid()) {
        m_services.render().set_viewport_size(RenderViewKind::Alternate,
            Vec2{static_cast<f32>(m_second_win.width()), static_cast<f32>(m_second_win.height())});
    }
    const bool mouse_valid = io.MousePos.x > -3.0e37f && io.MousePos.y > -3.0e37f;
    const RenderViewId main_view = m_services.render().first_active_main_view();
    if (main_view != 0 && io.DisplaySize.x > 0.f && io.DisplaySize.y > 0.f) {
        const float nx = std::clamp(io.MousePos.x / io.DisplaySize.x, 0.f, 1.f);
        const float ny = std::clamp(io.MousePos.y / io.DisplaySize.y, 0.f, 1.f);
        m_services.interaction().set_mouse(main_view,
            Vec2{io.MousePos.x, io.MousePos.y},
            Vec2{nx * 2.f - 1.f, 1.f - ny * 2.f},
            mouse_valid && !io.WantCaptureMouse);
    }
    const RenderViewId alternate_view = m_services.render().first_active_alternate_view();
    bool second_hovered = false;
    Vec2 second_delta{};
    if (m_second_win.valid() && alternate_view != 0 && m_second_win.width() > 0 && m_second_win.height() > 0) {
        const Vec2 pixel = m_second_win.cursor_position();
        second_hovered = m_second_win.hovered()
            && pixel.x >= 0.f && pixel.y >= 0.f
            && pixel.x <= static_cast<f32>(m_second_win.width())
            && pixel.y <= static_cast<f32>(m_second_win.height());
        const float nx = std::clamp(pixel.x / static_cast<f32>(m_second_win.width()), 0.f, 1.f);
        const float ny = std::clamp(pixel.y / static_cast<f32>(m_second_win.height()), 0.f, 1.f);
        m_services.interaction().set_mouse(alternate_view,
            pixel,
            Vec2{nx * 2.f - 1.f, 1.f - ny * 2.f},
            second_hovered);
        if (second_hovered && m_second_mouse_prev_valid)
            second_delta = pixel - m_second_mouse_prev;
        m_second_mouse_prev = pixel;
        m_second_mouse_prev_valid = second_hovered;
    } else {
        m_second_mouse_prev_valid = false;
    }

    if (io.WantCaptureMouse && !second_hovered) return;

    if (second_hovered && alternate_view != 0) {
        const Vec2 pixel = m_second_win.cursor_position();
        const float nx = std::clamp(pixel.x / static_cast<f32>(m_second_win.width()), 0.f, 1.f);
        const float ny = std::clamp(pixel.y / static_cast<f32>(m_second_win.height()), 0.f, 1.f);
        (void)m_services.camera_input().dispatch(
            m_services.camera(),
            m_services.interaction(),
            m_services.render(),
            CameraInputSample{
                .view = alternate_view,
                .profile = CameraViewProfile::Auto,
                .pixel = pixel,
                .normalized_pixel = {nx, ny},
                .screen_ndc = {nx * 2.f - 1.f, 1.f - ny * 2.f},
                .delta = second_delta,
                .wheel_delta = io.MouseWheel,
                .right_drag = m_second_win.mouse_button_down(GLFW_MOUSE_BUTTON_RIGHT),
                .middle_drag = m_second_win.mouse_button_down(GLFW_MOUSE_BUTTON_MIDDLE),
                .shift = io.KeyShift,
                .left_click = false,
                .left_double_click = false,
                .enabled = true
            });
        return;
    }

    if (main_view != 0 && io.DisplaySize.x > 0.f && io.DisplaySize.y > 0.f) {
        const float nx = std::clamp(io.MousePos.x / io.DisplaySize.x, 0.f, 1.f);
        const float ny = std::clamp(io.MousePos.y / io.DisplaySize.y, 0.f, 1.f);
        const bool double_click = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
        const bool left_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        (void)m_services.camera_input().dispatch(
            m_services.camera(),
            m_services.interaction(),
            m_services.render(),
            CameraInputSample{
                .view = main_view,
                .profile = CameraViewProfile::Auto,
                .pixel = {io.MousePos.x, io.MousePos.y},
                .normalized_pixel = {nx, ny},
                .screen_ndc = {nx * 2.f - 1.f, 1.f - ny * 2.f},
                .delta = {io.MouseDelta.x, io.MouseDelta.y},
                .wheel_delta = io.MouseWheel,
                .right_drag = ImGui::IsMouseDragging(ImGuiMouseButton_Right),
                .middle_drag = ImGui::IsMouseDragging(ImGuiMouseButton_Middle),
                .shift = io.KeyShift,
                .left_click = left_click,
                .left_double_click = double_click,
                .enabled = true,
                .perturb_seed = double_click ? m_surface_perturb_seed++ : 0u
            });
    }
}

void Engine::request_capture(bool pause_first) {
    if (pause_first)
        active_runtime().pause();
    m_event_log.push_back("PNG capture requested");
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
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    std::ostringstream stamp;
    stamp << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return std::filesystem::path{NDDE_PROJECT_DIR} / "captures" / (name + "_" + stamp.str() + ".png");
}

SimulationRuntime& Engine::active_runtime() {
    auto* runtime = m_simulations.get(m_active_sim);
    if (!runtime) throw std::runtime_error("[Engine] No active simulation runtime");
    return *runtime;
}

const SimulationRuntime& Engine::active_runtime() const {
    const auto* runtime = m_simulations.get(m_active_sim);
    if (!runtime) throw std::runtime_error("[Engine] No active simulation runtime");
    return *runtime;
}

void Engine::flush_render_service() {
    for (const RenderPacket& packet : m_services.render().packets()) {
        if (packet.vertices.empty()) continue;
        auto slice = m_services.memory().allocate_frame_vertices(static_cast<u32>(packet.vertices.size()));
        auto verts = slice.vertices();
        std::memcpy(verts, packet.vertices.data(), packet.vertices.size() * sizeof(Vertex));

        renderer::DrawCall dc{
            .slice = slice,
            .topology = packet.topology,
            .mode = packet.mode,
            .color = packet.color,
            .mvp = packet.mvp
        };

        const RenderTarget target = m_services.render().view_kind(packet.view) == RenderViewKind::Alternate
            ? RenderTarget::Contour2D
            : RenderTarget::Primary3D;
        switch (target) {
            case RenderTarget::Primary3D:
                m_renderer.draw(dc);
                break;
            case RenderTarget::Contour2D:
                if (m_second_win.valid()) m_second_win.draw(dc);
                break;
        }
    }
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
        return m_services.memory().allocate_frame_vertices(n);
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

    api.switch_simulation = [this](std::size_t index) {
        m_pending_sim = index;
    };

    return api;
}

} // namespace ndde
