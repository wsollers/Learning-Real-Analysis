#include "app/WavePredatorPreyPanels.hpp"

#include "app/SurfaceRegistry.hpp"
#include "app/SwarmRecipePanel.hpp"

#include <imgui.h>
#include <numbers>
#include <vector>

namespace ndde {

WavePredatorPreyPanels::WavePredatorPreyPanels(ParticleSystem& particles,
                                               WavePredatorPreySpawner& spawner,
                                               SurfaceMeshCache& mesh,
                                               Viewport& vp3d,
                                               SwarmBuildResult& last_swarm,
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
    , m_last_swarm(last_swarm)
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

void WavePredatorPreyPanels::draw_all() {
    m_panels.draw_all();
}

void WavePredatorPreyPanels::register_panels() {
    m_panels.clear();
    m_panels.add({
        .title = "Wave - Swarms",
        .default_pos = {20.f, 55.f},
        .default_size = {330.f, 455.f},
        .bg_alpha = 0.90f,
        .draw_body = [this] { draw_swarms_body(); }
    });
    m_panels.add({
        .title = "Wave - Camera",
        .default_pos = {20.f, 530.f},
        .default_size = {330.f, 150.f},
        .bg_alpha = 0.90f,
        .draw_body = [this] { draw_camera_body(); }
    });
}

void WavePredatorPreyPanels::draw_swarms_body() {
    const auto surface = SurfaceRegistry::describe(SurfaceKey::WavePredatorPrey);
    std::vector<SwarmRecipeAction> actions{
        {.label = "Reset predator/prey", .hotkey = "Ctrl+R", .spawn = [this] { return m_spawner.spawn_predator_prey_showcase(); }},
        {.label = "Add Brownian cloud", .hotkey = "Ctrl+B", .spawn = [this] { return m_spawner.spawn_brownian_cloud(); }},
        {.label = "Add contour band", .hotkey = "Ctrl+L", .spawn = [this] { return m_spawner.spawn_contour_band(); }},
        {.label = "Clear all", .hotkey = "", .spawn = [this] { return m_spawner.clear_all(); }}
    };
    SwarmRecipePanel::draw(SwarmRecipePanelState{
        .paused = &m_paused,
        .sim_speed = &m_sim_speed,
        .grid_lines = &m_grid_lines,
        .color_scale = &m_color_scale,
        .show_frenet = &m_show_frenet,
        .show_osculating_circle = &m_show_osc,
        .mesh = &m_mesh
    }, actions, m_particles, m_goal_status, &m_last_swarm, SwarmRecipePanelOptions{
        .surface_formula = surface.formula.data(),
        .grid_label = "Grid##wave",
        .color_scale_label = "Color scale##wave",
        .goal_success_text = "Predator capture reached - paused"
    });
}

void WavePredatorPreyPanels::draw_camera_body() {
    ImGui::SliderFloat("Yaw##wave", &m_vp3d.yaw, -std::numbers::pi_v<float>, std::numbers::pi_v<float>, "%.2f");
    ImGui::SliderFloat("Pitch##wave", &m_vp3d.pitch, -1.5f, 1.5f, "%.2f");
    ImGui::SliderFloat("Zoom##wave", &m_vp3d.zoom, 0.1f, 8.f, "%.2f");
    if (ImGui::Button("Reset##wave")) m_vp3d.reset();
}

} // namespace ndde
