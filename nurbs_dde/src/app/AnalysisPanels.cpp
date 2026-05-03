#include "app/AnalysisPanels.hpp"

#include "app/ParticleInspectorPanel.hpp"
#include "numeric/ops.hpp"

#include <algorithm>
#include <imgui.h>
#include <numbers>

namespace ndde {

AnalysisPanels::AnalysisPanels(ndde::math::SineRationalSurface& surface,
                               ParticleSystem& particles,
                               AnalysisSpawner& spawner,
                               SurfaceMeshCache& mesh,
                               Viewport& vp3d,
                               u32& grid_lines,
                               float& curv_scale,
                               bool& show_frenet,
                               bool& show_osc,
                               float& epsilon,
                               float& walk_speed,
                               float& noise_sigma,
                               bool& paused,
                               float& sim_speed,
                               u32& spawn_count,
                               GoalStatus& goal_status)
    : m_surface(surface)
    , m_particles(particles)
    , m_spawner(spawner)
    , m_mesh(mesh)
    , m_vp3d(vp3d)
    , m_grid_lines(grid_lines)
    , m_curv_scale(curv_scale)
    , m_show_frenet(show_frenet)
    , m_show_osc(show_osc)
    , m_epsilon(epsilon)
    , m_walk_speed(walk_speed)
    , m_noise_sigma(noise_sigma)
    , m_paused(paused)
    , m_sim_speed(sim_speed)
    , m_spawn_count(spawn_count)
    , m_goal_status(goal_status)
{
    register_panels();
}

void AnalysisPanels::draw_all() {
    m_panels.draw_all();
}

void AnalysisPanels::register_panels() {
    m_panels.clear();
    m_panels.add({
        .title = "Analysis - Surface",
        .default_pos = {20.f, 20.f},
        .default_size = {290.f, 340.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_surface_body(); }
    });
    m_panels.add({
        .title = "Analysis - Walkers",
        .default_pos = {20.f, 370.f},
        .default_size = {290.f, 380.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_walkers_body(); }
    });
    m_panels.add({
        .title = "Analysis - Camera",
        .default_pos = {20.f, 760.f},
        .default_size = {290.f, 160.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_camera_body(); }
    });
}

void AnalysisPanels::draw_surface_body() {
    ImGui::SeparatorText("Function");
    ImGui::TextDisabled("f(x,y) = [3/(1+(x+y+1)^2)] sin(2x)cos(2y)");
    ImGui::TextDisabled("       + 0.1 sin(5x) sin(5y)");
    ImGui::Spacing();

    ImGui::SeparatorText("Geometry");
    int gl = static_cast<int>(m_grid_lines);
    if (ImGui::SliderInt("Grid lines##as", &gl, 8, 120)) {
        m_grid_lines = static_cast<u32>(gl);
        m_mesh.mark_dirty();
    }
    if (ImGui::SliderFloat("Color scale##as", &m_curv_scale, 0.1f, 10.f, "%.2f")) {
        m_mesh.mark_dirty();
    }
    ImGui::TextDisabled("~%u k verts", (4u * m_grid_lines * (m_grid_lines + 1u)) / 1000u);
    ImGui::Checkbox("Hover Frenet  [Ctrl+F]", &m_show_frenet);
    ImGui::Checkbox("Osculating circle  [Ctrl+O]", &m_show_osc);

    ImGui::SeparatorText("At cursor (u,v)");
    static float probe_u = 0.f;
    static float probe_v = 0.f;
    ImGui::SetNextItemWidth(90.f);
    ImGui::InputFloat("u##probe", &probe_u, 0.1f, 0.5f, "%.2f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.f);
    ImGui::InputFloat("v##probe", &probe_v, 0.1f, 0.5f, "%.2f");
    probe_u = std::clamp(probe_u, m_surface.u_min(), m_surface.u_max());
    probe_v = std::clamp(probe_v, m_surface.v_min(), m_surface.v_max());

    const float z = m_surface.height(probe_u, probe_v);
    const float gx = m_surface.du(probe_u, probe_v).z;
    const float gy = m_surface.dv(probe_u, probe_v).z;
    const float gm = ops::sqrt(gx * gx + gy * gy);
    const float K = m_surface.gaussian_curvature(probe_u, probe_v);
    const float H = m_surface.mean_curvature(probe_u, probe_v);

    ImGui::TextDisabled("z     = %.4f", z);
    ImGui::TextDisabled("|grad| = %.4f", gm);
    ImGui::TextDisabled("grad  = (%.3f, %.3f)", gx, gy);
    ImGui::TextDisabled("K     = %.5f", K);
    ImGui::TextDisabled("H     = %.5f", H);
    const float perpx = -gy / (gm + 1e-9f);
    const float perpy = gx / (gm + 1e-9f);
    ImGui::TextDisabled("level-tangent = (%.3f, %.3f)", perpx, perpy);
}

void AnalysisPanels::draw_walkers_body() {
    ImGui::SeparatorText("Spawn parameters");
    ImGui::SliderFloat("Epsilon (band)##w", &m_epsilon, 0.02f, 1.f, "%.3f");
    ImGui::SliderFloat("Walk speed##w", &m_walk_speed, 0.1f, 2.f, "%.2f");
    ImGui::SliderFloat("Noise sigma##w", &m_noise_sigma, 0.f, 0.5f, "%.3f");
    ImGui::SliderFloat("Sim speed##w", &m_sim_speed, 0.1f, 5.f, "%.2f");
    ImGui::Spacing();

    if (ImGui::Button("Spawn walker  [Ctrl+W]", ImVec2(-1.f, 0.f))) m_spawner.spawn_walker();
    if (ImGui::Button("Reset showcase", ImVec2(-1.f, 0.f))) m_spawner.spawn_showcase_service();
    if (ImGui::Button("Clear all", ImVec2(-1.f, 0.f))) m_spawner.clear_all();

    if (m_goal_status == GoalStatus::Succeeded)
        ImGui::TextColored({0.4f, 1.f, 0.4f, 1.f}, "Capture reached - paused");
    ParticleInspectorPanel::draw(m_particles.particles(), ParticleInspectorOptions{
        .label = "Active walkers",
        .show_level_curve_controls = true,
        .show_brownian_controls = true,
        .show_trail_controls = true
    });
}

void AnalysisPanels::draw_camera_body() {
    ImGui::SliderFloat("Yaw##ac", &m_vp3d.yaw,
                       -std::numbers::pi_v<float>, std::numbers::pi_v<float>, "%.2f");
    ImGui::SliderFloat("Pitch##ac", &m_vp3d.pitch, -1.5f, 1.5f, "%.2f");
    ImGui::SliderFloat("Zoom##ac", &m_vp3d.zoom, 0.1f, 8.f, "%.2f");
    if (ImGui::Button("Reset##ac")) m_vp3d.reset();

    ImGui::SeparatorText("Pause");
    ImGui::Checkbox("Paused  [Ctrl+P]", &m_paused);
}

} // namespace ndde
