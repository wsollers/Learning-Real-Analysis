#include "app/AnalysisScene.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ndde {

AnalysisScene::AnalysisScene(EngineAPI api)
    : m_api(std::move(api))
    , m_surface(SurfaceRegistry::make_sine_rational(4.f))
    , m_particles(m_surface.get())
    , m_spawner(*m_surface, m_particles, m_spawn_count, m_epsilon, m_walk_speed,
                m_noise_sigma, m_sim_time, m_sim_speed, m_goal_status)
    , m_panels(*m_surface, m_particles, m_spawner, m_mesh, m_vp3d, m_grid_lines,
               m_curv_scale, m_show_frenet, m_show_osc, m_epsilon, m_walk_speed,
               m_noise_sigma, m_paused, m_sim_speed, m_spawn_count, m_goal_status)
{
    m_vp3d.base_extent = 5.f;
    m_vp3d.zoom = 1.f;
    m_vp3d.yaw = 0.6f;
    m_vp3d.pitch = 0.45f;

    register_bindings();
    m_spawner.spawn_showcase_service();
}

void AnalysisScene::on_frame(f32 dt) {
    advance_particles(m_particles, dt);
    draw_dockspace_root("##analysis_dock", "AnalysisDockSpace");

    const SurfaceSceneViewOptions view_options{
        .window_title = "Analysis 3D",
        .default_pos = {330.f, 20.f},
        .default_size = {750.f, 660.f},
        .canvas = {
            .grid_lines = m_grid_lines,
            .color_scale = m_curv_scale,
            .wire_color = {0.3f, 0.6f, 0.9f, 0.55f},
            .show_frenet = m_show_frenet,
            .show_osculating_circle = m_show_osc,
            .overlay_frame_scale = 0.32f,
            .canvas_id = "3d_canvas",
            .help_text = "Right-drag: orbit   Scroll: zoom   Ctrl+W: add walker",
            .subtitle = "3D projected surface preview",
            .paused = m_paused
        },
        .contour = {
            .draw_wire = true,
            .wire_color = {0.95f, 0.95f, 1.f, 0.32f},
            .trail_alpha_floor = 0.65f,
            .draw_heads = true
        }
    };
    m_view.draw_canvas(*m_surface, m_mesh, m_vp3d, m_particles, view_options,
        [this](ImDrawList& dl, ImVec2 cpos, ImVec2 csz) {
            for (const auto& c : m_particles.particles()) {
                if (!c.has_trail()) continue;
                if (const auto* eq = level_walker(c)) {
                    const float z0 = eq->params().z0;
                    const float eps = eq->params().epsilon;
                    const Vec3 hp = c.head_world();
                    const ImVec2 pp = ProjectedSurfaceCanvas::projected_to_canvas(
                        ProjectedSurfaceCanvas::project_point(hp, m_vp3d), cpos, csz, m_surface->extent());
                    const float dz = std::abs(hp.z - z0);
                    const float t = std::clamp(dz / (eps + 1e-6f), 0.f, 1.f);
                    dl.AddCircleFilled(pp, 3.5f,
                        IM_COL32(static_cast<int>((1.f - t) * 255.f),
                                 static_cast<int>((0.3f + 0.5f * (1.f - t)) * 255.f),
                                 static_cast<int>(t * 0.8f * 255.f),
                                 255),
                        14);
                }
            }
        });
    m_panels.draw_all();
    m_view.submit_contour(m_api, *m_surface, m_mesh, m_particles, view_options);
    draw_hotkey_panel(m_hotkeys);
}

void AnalysisScene::on_key_event(int key, int action, int mods) {
    (void)m_hotkeys.handle_key_event(key, action, mods);
}

void AnalysisScene::register_bindings() {
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_W), "Spawn walker",
        [this]{ m_spawner.spawn_walker(); }, "Walkers");
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_P), "Pause / unpause",
        [this]{ m_paused = !m_paused; }, "Simulation");
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_F), "Hover Frenet frame",
        m_show_frenet, "Overlays");
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_O), "Hover osculating circle",
        m_show_osc, "Overlays");
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_H), "Hotkey panel",
        m_show_hotkeys, "Panels");
}

ndde::sim::LevelCurveWalker* AnalysisScene::level_walker(AnimatedCurve& c) noexcept {
    return c.find_equation<ndde::sim::LevelCurveWalker>();
}

const ndde::sim::LevelCurveWalker* AnalysisScene::level_walker(const AnimatedCurve& c) noexcept {
    return c.find_equation<ndde::sim::LevelCurveWalker>();
}

} // namespace ndde
