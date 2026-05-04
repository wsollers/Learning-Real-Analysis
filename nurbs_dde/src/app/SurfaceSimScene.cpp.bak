// app/SurfaceSimScene.cpp
// Lifecycle/orchestration only. Panels, hotkeys, simulation setup, and render
// submission live in the SurfaceSimScene* companion translation units.
#include "app/SurfaceSimScene.hpp"
#include <imgui.h>
#include <string>
#include <utility>
#include <vector>

namespace ndde {
// ── Constructor ───────────────────────────────────────────────────────────────

SurfaceSimScene::SurfaceSimScene(EngineAPI api)
    : m_api(std::move(api))
    , m_surface(std::make_unique<GaussianSurface>())
    , m_particles(m_surface.get())
    , m_curves(m_particles.particles())
    , m_spawner(m_surface, m_particles, m_curves, m_behavior_params, m_pursuit,
                m_spawn, m_goal_status, m_sim_time, m_sim_speed)
    , m_controller(m_surface, m_particles, m_curves, m_surface_state, m_display,
                   m_pursuit, m_spawn, m_spawner, m_paused, m_sim_time, m_sim_speed)
    , m_panels(m_surface, m_particles, m_curves, m_vp3d, m_vp2d, m_hotkeys,
               m_surface_state, m_display, m_overlays, m_ui, m_spawn,
               m_behavior_params, m_pursuit, m_spawner, m_controller,
               m_paused, m_sim_time, m_sim_speed, m_goal_status,
               [this](const std::string& path) { export_session(path); })
    , m_view(m_api)
{
    m_vp3d.base_extent = 6.f;
    m_vp3d.zoom        = 1.0f;
    m_vp3d.yaw         = 0.55f;
    m_vp3d.pitch       = 0.42f;

    m_vp2d.base_extent = 6.f;
    m_vp2d.zoom        = 1.f;
    m_vp2d.pan_x       = 0.f;
    m_vp2d.pan_y       = 0.f;

    m_spawner.spawn_showcase_service();

    register_bindings();
}

// ── on_frame ──────────────────────────────────────────────────────────────────

void SurfaceSimScene::on_frame(f32 dt) {
    m_controller.advance_simulation(dt);
    update_snapshot_particles(m_particles);
    m_goal_status = m_particles.evaluate_goals(m_sim_time);
    if (m_goal_status == GoalStatus::Succeeded)
        m_paused = true;

    draw_dockspace_root("##surface_sim_dockspace_root", "SurfaceSimDockSpace");

    m_panels.draw_all();
    draw_surface_3d_window();
    submit_contour_second_window();
    draw_hotkey_panel();

    struct StubConic { std::string name; std::vector<int> snap_cache; };
    const std::vector<StubConic> no_conics;
    m_coord_debug.update(m_vp3d, m_hover_state.hover, no_conics, m_api.viewport_size());
    m_coord_debug.draw();
    m_ui.debug_open = m_coord_debug.visible();
}
} // namespace ndde
