#pragma once
// app/WavePredatorPreyScene.hpp
// Sim 4: trigonometric wave surface with predator/prey swarm recipes.

#include "engine/EngineAPI.hpp"
#include "app/ContourWindowRenderer.hpp"
#include "app/HotkeyManager.hpp"
#include "app/PanelHost.hpp"
#include "app/ParticleInspectorPanel.hpp"
#include "app/ProjectedSurfaceCanvas.hpp"
#include "app/ParticleSwarmFactory.hpp"
#include "app/SimulationSceneBase.hpp"
#include "app/SurfaceRegistry.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "app/SwarmRecipePanel.hpp"
#include "app/Viewport.hpp"
#include "math/GeometryTypes.hpp"
#include "numeric/ops.hpp"

#include <imgui.h>
#include <algorithm>
#include <memory>
#include <numbers>
#include <string_view>
#include <vector>

namespace ndde {

class WavePredatorPreyScene final : public SimulationSceneBase {
public:
    explicit WavePredatorPreyScene(EngineAPI api)
        : m_api(std::move(api))
        , m_surface(SurfaceRegistry::make_wave_predator_prey(4.f))
        , m_particles(m_surface.get(), 4242u)
    {
        m_vp3d.base_extent = 5.f;
        m_vp3d.zoom = 1.f;
        m_vp3d.yaw = 0.58f;
        m_vp3d.pitch = 0.50f;

        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_R), "Reset predator/prey",
            [this]{ m_last_swarm = spawn_predator_prey_showcase(); }, "Swarms");
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_B), "Brownian cloud",
            [this]{ m_last_swarm = spawn_brownian_cloud(); }, "Swarms");
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_L), "Contour band",
            [this]{ m_last_swarm = spawn_contour_band(); }, "Swarms");
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_P), "Pause / unpause",
            [this]{ m_paused = !m_paused; }, "Simulation");
        m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_F), "Hover Frenet frame",
            m_show_frenet, "Overlays");
        m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_O), "Hover osculating circle",
            m_show_osc, "Overlays");
        m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_H), "Hotkey panel",
            m_show_hotkeys, "Panels");

        register_panels();
        m_last_swarm = spawn_predator_prey_showcase();
    }

    void on_frame(f32 dt) override {
        advance_particles(m_particles, dt);
        draw_dockspace_root("##wave_predator_dock", "WavePredatorDockSpace");

        draw_canvas_3d();
        m_panels.draw_all();
        submit_contour_second_window();
        draw_hotkey_panel(m_hotkeys);
    }

    [[nodiscard]] std::string_view name() const override { return "Wave Predator-Prey"; }

    void on_key_event(int key, int action, int mods) override {
        (void)m_hotkeys.handle_key_event(key, action, mods);
    }

private:
    EngineAPI m_api;
    std::unique_ptr<WavePredatorPreySurface> m_surface;
    ParticleSystem m_particles;
    HotkeyManager m_hotkeys;
    PanelHost m_panels;
    Viewport m_vp3d;
    SurfaceMeshCache m_mesh;

    u32 m_grid_lines = 80;
    float m_color_scale = 1.05f;
    bool m_show_frenet = true;
    bool m_show_osc = true;
    SwarmBuildResult m_last_swarm;

    void register_panels() {
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

    void reset_particles() {
        m_particles.clear();
        m_particles.clear_goals();
        m_goal_status = GoalStatus::Running;
        reset_simulation_clock();
    }

    [[nodiscard]] SwarmBuildResult spawn_predator_prey_showcase() {
        reset_particles();
        ParticleSwarmFactory swarms(m_particles);
        SwarmBuildResult swarm = swarms.leader_pursuit(ParticleSwarmFactory::LeaderPursuitParams{
            .leader_count = 3u,
            .chaser_count = 9u,
            .center = {0.f, 0.f},
            .leader_radius = 0.85f,
            .chaser_radius = 2.85f,
            .leader_speed = 0.48f,
            .chaser_speed = 0.92f,
            .delay_seconds = 0.42f,
            .capture_radius = 0.18f,
            .leader_noise = {.sigma = 0.09f, .drift_strength = -0.04f},
            .chaser_noise = {.sigma = 0.05f, .drift_strength = 0.f},
            .leader_trail = {TrailMode::Persistent, 2200u, 0.010f},
            .chaser_trail = {TrailMode::Finite, 1400u, 0.012f},
            .add_capture_goal = true,
            .leader_label = "Prey - Delayed Avoid - Brownian",
            .chaser_label = "Predator - Delayed Seek - Brownian"
        });

        for (int i = 0; i < 100; ++i) {
            m_sim_time += 1.f / 60.f;
            m_particles.update(1.f / 60.f, 1.f, m_sim_time);
        }
        return swarm;
    }

    [[nodiscard]] SwarmBuildResult spawn_brownian_cloud() {
        ParticleSwarmFactory swarms(m_particles);
        return swarms.brownian_cloud(ParticleSwarmFactory::BrownianCloudParams{
            .count = 18u,
            .center = {0.f, 0.f},
            .radius = 2.2f,
            .role = ParticleRole::Avoider,
            .brownian = {.sigma = 0.13f, .drift_strength = 0.025f},
            .trail = {TrailMode::Finite, 900u, 0.014f},
            .label = "Avoider - Brownian Cloud"
        });
    }

    [[nodiscard]] SwarmBuildResult spawn_contour_band() {
        ParticleSwarmFactory swarms(m_particles);
        ndde::sim::LevelCurveWalker::Params walker;
        walker.epsilon = 0.16f;
        walker.walk_speed = 0.54f;
        walker.turn_rate = 2.7f;
        walker.tangent_floor = 0.45f;
        return swarms.contour_band(ParticleSwarmFactory::ContourBandParams{
            .count = 12u,
            .center = {0.f, 0.f},
            .radius = 1.8f,
            .shared_level = true,
            .role = ParticleRole::Neutral,
            .walker = walker,
            .noise = {.sigma = 0.018f, .drift_strength = 0.f},
            .trail = {TrailMode::Persistent, 2200u, 0.010f},
            .label = "Neutral - Contour Band - Brownian"
        });
    }

    void rebuild_geometry_if_needed() {
        m_mesh.rebuild_if_needed(*m_surface, SurfaceMeshOptions{
            .grid_lines = m_grid_lines,
            .time = 0.f,
            .color_scale = m_color_scale,
            .wire_color = {0.92f, 0.96f, 1.f, 0.30f},
            .fill_color_mode = SurfaceFillColorMode::HeightCell,
            .build_contour = true
        });
    }

    void draw_canvas_3d() {
        ImGui::SetNextWindowPos(ImVec2(360.f, 20.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(900.f, 720.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.f);
        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground;
        ImGui::Begin("Wave Predator-Prey 3D", nullptr, flags);
        ProjectedSurfaceCanvas::draw(*m_surface, m_mesh, m_vp3d, m_particles, ProjectedSurfaceCanvasOptions{
            .grid_lines = m_grid_lines,
            .color_scale = m_color_scale,
            .wire_color = {0.92f, 0.96f, 1.f, 0.30f},
            .show_frenet = m_show_frenet,
            .show_osculating_circle = m_show_osc,
            .overlay_frame_scale = 0.38f,
            .canvas_id = "wave_predator_canvas",
            .help_text = "Right-drag: orbit   Scroll: zoom   Ctrl+R: reset   Ctrl+B: cloud   Ctrl+L: contour band",
            .paused = m_paused
        });
        ImGui::End();
    }

    void submit_contour_second_window() {
        rebuild_geometry_if_needed();
        submit_contour_window(m_api, m_mesh, m_particles, ContourWindowOptions{
            .extent = m_surface->extent(),
            .trail_alpha_floor = 0.72f
        });
    }

    void draw_swarms_body() {
        const auto surface = SurfaceRegistry::describe(SurfaceKey::WavePredatorPrey);
        std::vector<SwarmRecipeAction> actions{
            {.label = "Reset predator/prey", .hotkey = "Ctrl+R", .spawn = [this] { return spawn_predator_prey_showcase(); }},
            {.label = "Add Brownian cloud", .hotkey = "Ctrl+B", .spawn = [this] { return spawn_brownian_cloud(); }},
            {.label = "Add contour band", .hotkey = "Ctrl+L", .spawn = [this] { return spawn_contour_band(); }},
            {.label = "Clear all", .hotkey = "", .spawn = [this] {
                reset_particles();
                return SwarmBuildResult{.metadata = SwarmRecipeMetadata{
                    .family_name = "Clear All",
                    .requested_count = 0u,
                    .roles_emitted = {},
                    .goals_added = false
                }};
            }}
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

    void draw_camera_body() {
        ImGui::SliderFloat("Yaw##wave", &m_vp3d.yaw, -std::numbers::pi_v<float>, std::numbers::pi_v<float>, "%.2f");
        ImGui::SliderFloat("Pitch##wave", &m_vp3d.pitch, -1.5f, 1.5f, "%.2f");
        ImGui::SliderFloat("Zoom##wave", &m_vp3d.zoom, 0.1f, 8.f, "%.2f");
        if (ImGui::Button("Reset##wave")) m_vp3d.reset();
    }
};

} // namespace ndde
