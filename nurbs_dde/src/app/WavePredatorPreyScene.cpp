#include "app/WavePredatorPreyScene.hpp"

#include <utility>

namespace ndde {

WavePredatorPreyScene::WavePredatorPreyScene(EngineAPI api)
    : m_api(std::move(api))
    , m_surface(SurfaceRegistry::make_wave_predator_prey(4.f))
    , m_particles(m_surface.get(), 4242u)
    , m_spawner(m_particles, m_sim_time, m_goal_status)
    , m_panels(m_particles, m_spawner, m_mesh, m_vp3d, m_last_swarm, m_grid_lines,
               m_color_scale, m_show_frenet, m_show_osc, m_paused, m_sim_speed, m_goal_status)
{
    m_vp3d.base_extent = 5.f;
    m_vp3d.zoom = 1.f;
    m_vp3d.yaw = 0.58f;
    m_vp3d.pitch = 0.50f;

    register_bindings();
    m_last_swarm = m_spawner.spawn_predator_prey_showcase();
}

void WavePredatorPreyScene::on_frame(f32 dt) {
    advance_particles(m_particles, dt);
    draw_dockspace_root("##wave_predator_dock", "WavePredatorDockSpace");

    m_view.draw_canvas(*m_surface, m_mesh, m_vp3d, m_particles, SurfaceSceneViewOptions{
        .window_title = "Wave Predator-Prey 3D",
        .default_pos = {360.f, 20.f},
        .default_size = {900.f, 720.f},
        .canvas = {
            .grid_lines = m_grid_lines,
            .color_scale = m_color_scale,
            .wire_color = {0.92f, 0.96f, 1.f, 0.30f},
            .show_frenet = m_show_frenet,
            .show_osculating_circle = m_show_osc,
            .overlay_frame_scale = 0.38f,
            .canvas_id = "wave_predator_canvas",
            .help_text = "Right-drag: orbit   Scroll: zoom   Ctrl+R: reset   Ctrl+B: cloud   Ctrl+L: contour band",
            .paused = m_paused
        },
        .contour = {
            .trail_alpha_floor = 0.72f
        }
    });
    m_panels.draw_all();
    m_view.submit_contour(m_api, *m_surface, m_mesh, m_particles, SurfaceSceneViewOptions{
        .canvas = {
            .grid_lines = m_grid_lines,
            .color_scale = m_color_scale,
            .wire_color = {0.92f, 0.96f, 1.f, 0.30f}
        },
        .contour = {
            .trail_alpha_floor = 0.72f
        }
    });
    draw_hotkey_panel(m_hotkeys);
}

void WavePredatorPreyScene::on_key_event(int key, int action, int mods) {
    (void)m_hotkeys.handle_key_event(key, action, mods);
}

void WavePredatorPreyScene::register_bindings() {
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_R), "Reset predator/prey",
        [this]{ m_last_swarm = m_spawner.spawn_predator_prey_showcase(); }, "Swarms");
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_B), "Brownian cloud",
        [this]{ m_last_swarm = m_spawner.spawn_brownian_cloud(); }, "Swarms");
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_L), "Contour band",
        [this]{ m_last_swarm = m_spawner.spawn_contour_band(); }, "Swarms");
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_P), "Pause / unpause",
        [this]{ m_paused = !m_paused; }, "Simulation");
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_F), "Hover Frenet frame",
        m_show_frenet, "Overlays");
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_O), "Hover osculating circle",
        m_show_osc, "Overlays");
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_H), "Hotkey panel",
        m_show_hotkeys, "Panels");
}

} // namespace ndde
