#include "app/MultiWellPanels.hpp"

#include "app/ParticleInspectorPanel.hpp"

#include <algorithm>
#include <imgui.h>
#include <numbers>

namespace ndde {

MultiWellPanels::MultiWellPanels(ParticleSystem& particles,
                                 MultiWellSpawner& spawner,
                                 SurfaceMeshCache& mesh,
                                 Viewport& vp3d,
                                 u32& grid_lines,
                                 float& color_scale,
                                 bool& show_frenet,
                                 bool& show_osc,
                                 bool& paused,
                                 float& sim_speed,
                                 GoalStatus& goal_status)
    : m_particles(particles)
    , m_spawner(spawner)
    , m_mesh(mesh)
    , m_vp3d(vp3d)
    , m_grid_lines(grid_lines)
    , m_color_scale(color_scale)
    , m_show_frenet(show_frenet)
    , m_show_osc(show_osc)
    , m_paused(paused)
    , m_sim_speed(sim_speed)
    , m_goal_status(goal_status)
{
    register_panels();
}

void MultiWellPanels::draw_all() {
    m_panels.draw_all();
}

void MultiWellPanels::register_panels() {
    m_panels.clear();
    m_panels.add({
        .title = "Multi-Well - Particles",
        .default_pos = {20.f, 55.f},
        .default_size = {310.f, 430.f},
        .bg_alpha = 0.90f,
        .draw_body = [this] { draw_particles_body(); }
    });
    m_panels.add({
        .title = "Multi-Well - Camera",
        .default_pos = {20.f, 505.f},
        .default_size = {310.f, 150.f},
        .bg_alpha = 0.90f,
        .draw_body = [this] { draw_camera_body(); }
    });
}

void MultiWellPanels::draw_particles_body() {
    ImGui::Checkbox("Paused  [Ctrl+P]", &m_paused);
    ImGui::SliderFloat("Sim speed##mw", &m_sim_speed, 0.1f, 5.f, "%.2f");
    int grid = static_cast<int>(m_grid_lines);
    if (ImGui::SliderInt("Grid##mw", &grid, 12, 140)) {
        m_grid_lines = static_cast<u32>(std::max(grid, 12));
        m_mesh.mark_dirty();
    }
    ImGui::SliderFloat("Color scale##mw", &m_color_scale, 0.2f, 4.f, "%.2f");
    ImGui::Checkbox("Hover Frenet  [Ctrl+F]", &m_show_frenet);
    ImGui::Checkbox("Osculating circle  [Ctrl+O]", &m_show_osc);
    if (ImGui::Button("Spawn avoider  [Ctrl+A]", ImVec2(-1.f, 0.f))) m_spawner.spawn_avoider();
    if (ImGui::Button("Spawn centroid seeker  [Ctrl+C]", ImVec2(-1.f, 0.f))) m_spawner.spawn_centroid_seeker();
    if (ImGui::Button("Reset showcase  [Ctrl+R]", ImVec2(-1.f, 0.f))) m_spawner.spawn_showcase_service();
    if (ImGui::Button("Clear all", ImVec2(-1.f, 0.f))) m_spawner.clear_all();
    if (m_goal_status == GoalStatus::Succeeded)
        ImGui::TextColored({0.4f, 1.f, 0.4f, 1.f}, "Capture reached - paused");
    ParticleInspectorPanel::draw(m_particles.particles(), ParticleInspectorOptions{
        .label = "Active particles",
        .show_level_curve_controls = false,
        .show_brownian_controls = true,
        .show_trail_controls = true
    });
}

void MultiWellPanels::draw_camera_body() {
    ImGui::SliderFloat("Yaw##mw", &m_vp3d.yaw, -std::numbers::pi_v<float>, std::numbers::pi_v<float>, "%.2f");
    ImGui::SliderFloat("Pitch##mw", &m_vp3d.pitch, -1.5f, 1.5f, "%.2f");
    ImGui::SliderFloat("Zoom##mw", &m_vp3d.zoom, 0.1f, 8.f, "%.2f");
    if (ImGui::Button("Reset##mw")) m_vp3d.reset();
}

} // namespace ndde
