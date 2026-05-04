#include "app/SimulationWavePredatorPrey.hpp"

#include "app/AlternateViewPanel.hpp"
#include "app/SimulationRenderPackets.hpp"

#include <imgui.h>
#include <vector>

namespace ndde {

SimulationWavePredatorPrey::SimulationWavePredatorPrey()
    : m_surface(SurfaceRegistry::make_wave_predator_prey(4.f))
    , m_particles(m_surface.get(), 4242u)
    , m_spawner(m_particles, m_sim_time, m_goal_status)
{
    sync_context();
}

void SimulationWavePredatorPrey::on_register(SimulationHost& host) {
    m_host = &host;
    m_panel_handles.add(host.panels().register_panel(PanelDescriptor{
        .title = "Sim - Controls",
        .category = "Simulation",
        .scope = PanelScope::Simulation,
        .draw = [this] { draw_control_panel(); }
    }));
    m_panel_handles.add(host.panels().register_panel(PanelDescriptor{
        .title = "Sim - Swarms",
        .category = "Simulation",
        .scope = PanelScope::Simulation,
        .draw = [this] { draw_swarm_panel(); }
    }));
    m_panel_handles.add(host.panels().register_panel(PanelDescriptor{
        .title = "Sim - Particles",
        .category = "Simulation",
        .scope = PanelScope::Simulation,
        .draw = [this] { draw_particle_panel(); }
    }));
    m_panel_handles.add(host.panels().register_panel(PanelDescriptor{
        .title = "Sim - Goals",
        .category = "Simulation",
        .scope = PanelScope::Simulation,
        .draw = [this] { draw_goal_panel(); }
    }));
    m_reset_hotkey = host.hotkeys().register_action(HotkeyDescriptor{
        .chord = {.key = 'R', .mods = 2},
        .label = "Reset predator/prey",
        .callback = [this] { reset_showcase(); }
    });
    m_cloud_hotkey = host.hotkeys().register_action(HotkeyDescriptor{
        .chord = {.key = 'B', .mods = 2},
        .label = "Wave Brownian cloud",
        .callback = [this] { spawn_cloud(); }
    });
    m_contour_hotkey = host.hotkeys().register_action(HotkeyDescriptor{
        .chord = {.key = 'L', .mods = 2},
        .label = "Wave contour band",
        .callback = [this] { spawn_contour_band(); }
    });
    m_main_handle = host.render().register_view(RenderViewDescriptor{
        .title = "Wave Predator-Prey 3D",
        .kind = RenderViewKind::Main,
        .overlays = {.show_axes = true, .show_grid = true, .show_hover_frenet = true, .show_osculating_circle = true}
    }, &m_main_view);
    m_alt_handle = host.render().register_view(RenderViewDescriptor{
        .title = "Wave Predator-Prey Alternate",
        .kind = RenderViewKind::Alternate,
        .alternate_mode = AlternateViewMode::Flow,
        .projection = CameraProjection::Orthographic,
        .overlays = {.show_axes = true},
        .alternate = {
            .isocline_direction_angle = 0.25f,
            .isocline_target_slope = 0.f,
            .isocline_tolerance = 0.75f,
            .isocline_bands = 9u,
            .vector_mode = VectorFieldMode::LevelTangent,
            .vector_samples = 20u,
            .vector_scale = 1.1f,
            .flow_seed_count = 11u,
            .flow_steps = 48u,
            .flow_step_size = 0.075f
        }
    }, &m_alternate_view);
}

void SimulationWavePredatorPrey::on_start() {
    reset_showcase();
}

void SimulationWavePredatorPrey::on_tick(const TickInfo& tick) {
    m_context.set_tick(tick);
    if (!tick.paused && !m_paused) {
        m_sim_time = tick.time;
        m_particles.update(tick.dt, m_sim_speed, m_sim_time);
        m_context.dirty().mark_particles_changed();
    }
    submit_geometry();
}

void SimulationWavePredatorPrey::on_stop() {
    m_panel_handles.clear();
    m_cloud_hotkey.reset();
    m_contour_hotkey.reset();
    m_reset_hotkey.reset();
    m_main_handle.reset();
    m_alt_handle.reset();
    m_host = nullptr;
}

SceneSnapshot SimulationWavePredatorPrey::snapshot() const {
    return SceneSnapshot{
        .name = std::string(name()),
        .paused = m_paused,
        .sim_time = m_sim_time,
        .sim_speed = m_sim_speed,
        .particle_count = m_particles.size(),
        .status = m_goal_status == GoalStatus::Succeeded ? "Succeeded" : "Running",
        .particles = m_particles.snapshot_particles()
    };
}

SimulationMetadata SimulationWavePredatorPrey::metadata() const {
    const ndde::math::SurfaceMetadata surface = m_surface->metadata(m_sim_time);
    return SimulationMetadata{
        .name = std::string(name()),
        .surface_name = std::string(surface.name),
        .surface_formula = std::string(surface.formula),
        .status = m_goal_status == GoalStatus::Succeeded ? "Succeeded" : "Running",
        .sim_time = m_sim_time,
        .sim_speed = m_sim_speed,
        .particle_count = m_particles.size(),
        .paused = m_paused,
        .goal_succeeded = m_goal_status == GoalStatus::Succeeded,
        .surface_has_analytic_derivatives = surface.has_analytic_derivatives,
        .surface_deformable = surface.deformable,
        .surface_time_varying = surface.time_varying
    };
}

void SimulationWavePredatorPrey::sync_context() {
    m_particles.set_surface(m_surface.get());
    m_context.set_surface(m_surface.get());
    m_context.set_particles(&m_particles.particles());
    m_context.set_rng(&m_particles.rng());
    m_context.set_time(m_sim_time);
}

void SimulationWavePredatorPrey::reset_showcase() {
    m_last_swarm = m_spawner.spawn_predator_prey_showcase();
    sync_context();
}

void SimulationWavePredatorPrey::spawn_cloud() {
    m_last_swarm = m_spawner.spawn_brownian_cloud();
    sync_context();
}

void SimulationWavePredatorPrey::spawn_contour_band() {
    m_last_swarm = m_spawner.spawn_contour_band();
    sync_context();
}

void SimulationWavePredatorPrey::draw_control_panel() {
    ImGui::SetNextWindowPos(ImVec2(24.f, 72.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300.f, 200.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Sim - Controls")) { ImGui::End(); return; }
    SimulationControlPanel::draw(metadata(), SimulationControls{
        .paused = &m_paused,
        .sim_speed = &m_sim_speed,
        .reset = [this] { reset_showcase(); }
    });
    if (m_host) AlternateViewPanel::draw(m_host->render(), m_alternate_view);
    ImGui::End();
}

void SimulationWavePredatorPrey::draw_swarm_panel() {
    ImGui::SetNextWindowPos(ImVec2(24.f, 300.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300.f, 230.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Sim - Swarms")) { ImGui::End(); return; }
    std::vector<SwarmRecipeAction> actions{
        {.label = "Reset predator/prey", .hotkey = "Ctrl+R", .spawn = [this] { reset_showcase(); return m_last_swarm; }},
        {.label = "Brownian cloud", .hotkey = "Ctrl+B", .spawn = [this] { spawn_cloud(); return m_last_swarm; }},
        {.label = "Contour band", .hotkey = "Ctrl+L", .spawn = [this] { spawn_contour_band(); return m_last_swarm; }}
    };
    SwarmRecipePanel::draw(SwarmRecipePanelState{}, actions, m_particles, m_goal_status, &m_last_swarm,
        SwarmRecipePanelOptions{.show_sim_controls = false, .show_particle_inspector = false});
    ImGui::End();
}

void SimulationWavePredatorPrey::draw_particle_panel() {
    ImGui::SetNextWindowPos(ImVec2(342.f, 72.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420.f, 480.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Sim - Particles")) { ImGui::End(); return; }
    ParticleInspectorPanel::draw(m_particles.particles());
    ImGui::End();
}

void SimulationWavePredatorPrey::draw_goal_panel() {
    ImGui::SetNextWindowPos(ImVec2(24.f, 548.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300.f, 130.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Sim - Goals")) { ImGui::End(); return; }
    GoalStatusPanel::draw(metadata());
    ImGui::End();
}

void SimulationWavePredatorPrey::submit_geometry() {
    if (!m_host) return;
    submit_surface_sim_packets(m_host->render(), m_main_view, m_alternate_view,
        *m_surface, m_mesh, m_particles, SurfaceMeshOptions{
            .grid_lines = 80u,
            .time = 0.f,
            .color_scale = 1.05f,
            .wire_color = {0.92f, 0.96f, 1.f, 0.30f},
            .fill_color_mode = SurfaceFillColorMode::HeightCell,
            .build_contour = true
        }, &m_host->interaction());
}

} // namespace ndde
