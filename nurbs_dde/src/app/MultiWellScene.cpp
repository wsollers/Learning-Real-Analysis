#include "app/MultiWellScene.hpp"

#include <utility>

namespace ndde {

MultiWellScene::MultiWellScene(EngineAPI api)
    : m_api(std::move(api))
    , m_surface(SurfaceRegistry::make_multi_well(4.f))
    , m_particles(m_surface.get())
    , m_spawner(m_particles, m_spawn_count, m_sim_time, m_goal_status)
    , m_panels(m_particles, m_spawner, m_mesh, m_vp3d, m_grid_lines, m_color_scale,
               m_show_frenet, m_show_osc, m_paused, m_sim_speed, m_goal_status)
{
    m_vp3d.base_extent = 5.f;
    m_vp3d.zoom = 1.f;
    m_vp3d.yaw = 0.48f;
    m_vp3d.pitch = 0.55f;

    register_bindings();
    m_spawner.spawn_showcase_service();
}

void MultiWellScene::on_frame(f32 dt) {
    advance_particles(m_particles, dt);
    draw_dockspace_root("##multiwell_dock", "MultiWellDockSpace");

    m_view.draw_canvas(*m_surface, m_mesh, m_vp3d, m_particles, SurfaceSceneViewOptions{
        .window_title = "Multi-Well 3D",
        .default_pos = {330.f, 20.f},
        .default_size = {850.f, 700.f},
        .canvas = {
            .grid_lines = m_grid_lines,
            .color_scale = m_color_scale,
            .wire_color = {0.92f, 0.96f, 1.f, 0.32f},
            .show_frenet = m_show_frenet,
            .show_osculating_circle = m_show_osc,
            .overlay_frame_scale = 0.34f,
            .canvas_id = "multiwell_canvas",
            .help_text = "Right-drag: orbit   Scroll: zoom   Ctrl+A: avoider   Ctrl+C: centroid seeker",
            .paused = m_paused
        },
        .contour = {
            .trail_alpha_floor = 0.70f
        }
    });
    m_panels.draw_all();
    m_view.submit_contour(m_api, *m_surface, m_mesh, m_particles, SurfaceSceneViewOptions{
        .canvas = {
            .grid_lines = m_grid_lines,
            .color_scale = m_color_scale,
            .wire_color = {0.92f, 0.96f, 1.f, 0.32f}
        },
        .contour = {
            .trail_alpha_floor = 0.70f
        }
    });
    draw_hotkey_panel(m_hotkeys);
}

void MultiWellScene::on_key_event(int key, int action, int mods) {
    (void)m_hotkeys.handle_key_event(key, action, mods);
}

void MultiWellScene::register_bindings() {
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_A), "Spawn avoider",
        [this]{ m_spawner.spawn_avoider(); }, "Particles");
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_C), "Spawn centroid seeker",
        [this]{ m_spawner.spawn_centroid_seeker(); }, "Particles");
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_R), "Reset showcase",
        [this]{ m_spawner.spawn_showcase_service(); }, "Simulation");
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
