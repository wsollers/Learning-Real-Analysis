#pragma once
// app/MultiWellScene.hpp
// Sim 3: multi-well Gaussian/trigonometric landscape with centroid pursuit.

#include "engine/EngineAPI.hpp"
#include "app/HotkeyManager.hpp"
#include "app/PanelHost.hpp"
#include "app/SimulationSceneBase.hpp"
#include "app/ContourWindowRenderer.hpp"
#include "app/ParticleBehaviors.hpp"
#include "app/ParticleInspectorPanel.hpp"
#include "app/ProjectedSurfaceCanvas.hpp"
#include "app/ParticleSystem.hpp"
#include "app/SurfaceRegistry.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "app/Viewport.hpp"
#include "math/GeometryTypes.hpp"
#include "numeric/ops.hpp"

#include <imgui.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <memory>
#include <numbers>
#include <string_view>

namespace ndde {

class MultiWellScene final : public SimulationSceneBase {
public:
    explicit MultiWellScene(EngineAPI api)
        : m_api(std::move(api))
        , m_surface(SurfaceRegistry::make_multi_well(4.f))
        , m_particles(m_surface.get())
    {
        m_vp3d.base_extent = 5.f;
        m_vp3d.zoom = 1.f;
        m_vp3d.yaw = 0.48f;
        m_vp3d.pitch = 0.55f;

        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_A), "Spawn avoider",
            [this]{ spawn_avoider(); }, "Particles");
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_C), "Spawn centroid seeker",
            [this]{ spawn_centroid_seeker(); }, "Particles");
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_R), "Reset showcase",
            [this]{ spawn_showcase_service(); }, "Simulation");
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_P), "Pause / unpause",
            [this]{ m_paused = !m_paused; }, "Simulation");
        m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_F), "Hover Frenet frame",
            m_show_frenet, "Overlays");
        m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_O), "Hover osculating circle",
            m_show_osc, "Overlays");
        m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_H), "Hotkey panel",
            m_show_hotkeys, "Panels");

        register_panels();
        spawn_showcase_service();
    }

    void on_frame(f32 dt) override {
        advance_particles(m_particles, dt);
        draw_dockspace_root("##multiwell_dock", "MultiWellDockSpace");

        draw_canvas_3d();
        m_panels.draw_all();
        submit_contour_second_window();
        draw_hotkey_panel(m_hotkeys);
    }

    [[nodiscard]] std::string_view name() const override { return "Multi-Well Centroid"; }

    void on_key_event(int key, int action, int mods) override {
        if (action != GLFW_PRESS) return;
        const bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
        const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
        if (!ctrl || shift) return;
        switch (key) {
            case GLFW_KEY_A: spawn_avoider(); break;
            case GLFW_KEY_C: spawn_centroid_seeker(); break;
            case GLFW_KEY_R: spawn_showcase_service(); break;
            case GLFW_KEY_P: m_paused = !m_paused; break;
            case GLFW_KEY_F: m_show_frenet = !m_show_frenet; break;
            case GLFW_KEY_O: m_show_osc = !m_show_osc; break;
            case GLFW_KEY_H: m_show_hotkeys = !m_show_hotkeys; break;
            default: break;
        }
    }

private:
    EngineAPI m_api;
    std::unique_ptr<MultiWellWaveSurface> m_surface;
    ParticleSystem m_particles;
    HotkeyManager m_hotkeys;
    PanelHost m_panels;
    Viewport m_vp3d;

    u32 m_grid_lines = 70;
    float m_color_scale = 1.25f;
    u32 m_spawn_count = 0;
    bool m_show_frenet = true;
    bool m_show_osc = true;

    SurfaceMeshCache m_mesh;

    void register_panels() {
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

    void spawn_showcase_service() {
        m_particles.clear();
        m_particles.clear_goals();
        m_spawn_count = 0;
        reset_simulation_clock();

        spawn_avoider_at({-2.15f, -1.15f}, {0.18f, 0.05f});
        spawn_avoider_at({ 1.55f,  1.35f}, {-0.12f, 0.10f});

        for (int i = 0; i < 90; ++i) {
            m_sim_time += 1.f / 60.f;
            m_particles.update(1.f / 60.f, 1.f, m_sim_time);
        }

        spawn_centroid_seeker_at({-0.20f, 2.55f});
        m_particles.add_goal<CaptureGoal>(CaptureGoal::Params{
            .seeker_role = ParticleRole::Chaser,
            .target_role = ParticleRole::Avoider,
            .radius = 0.20f
        });
        m_goal_status = GoalStatus::Running;
    }

    void spawn_avoider() {
        const float a = static_cast<float>(m_spawn_count) * 1.73f;
        const float r = 1.3f + 0.25f * static_cast<float>(m_spawn_count % 3u);
        spawn_avoider_at({r * ops::cos(a), r * ops::sin(a)}, {0.10f * ops::cos(a + 1.f), 0.10f * ops::sin(a + 1.f)});
    }

    void spawn_avoider_at(glm::vec2 uv, glm::vec2 drift) {
        AvoidParticleBehavior::Params avoid;
        avoid.target = TargetSelector::nearest(ParticleRole::Chaser);
        avoid.speed = 0.55f;
        avoid.delay_seconds = 0.35f;

        ParticleBuilder builder = m_particles.factory().particle();
        builder
            .named("Avoider - Delayed Avoid - Brownian")
            .role(ParticleRole::Avoider)
            .at(uv)
            .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.012f})
            .stochastic()
            .with_behavior<AvoidParticleBehavior>(avoid)
            .with_behavior<ConstantDriftBehavior>(0.35f, drift)
            .with_behavior<BrownianBehavior>(0.35f, BrownianBehavior::Params{
                .sigma = 0.10f,
                .drift_strength = 0.04f
            });
        m_particles.spawn(std::move(builder));
        ++m_spawn_count;
    }

    void spawn_centroid_seeker() {
        spawn_centroid_seeker_at({0.f, 0.f});
    }

    void spawn_centroid_seeker_at(glm::vec2 uv) {
        ParticleBuilder builder = m_particles.factory().particle();
        builder
            .named("Chaser - Centroid Seek - Brownian")
            .role(ParticleRole::Chaser)
            .at(uv)
            .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.012f})
            .stochastic()
            .with_behavior<CentroidSeekBehavior>(CentroidSeekBehavior::Params{
                .role = ParticleRole::Avoider,
                .speed = 0.78f
            })
            .with_behavior<BrownianBehavior>(0.18f, BrownianBehavior::Params{
                .sigma = 0.055f,
                .drift_strength = 0.f
            });
        m_particles.spawn(std::move(builder));
        ++m_spawn_count;
    }

    void rebuild_geometry_if_needed() {
        m_mesh.rebuild_if_needed(*m_surface, SurfaceMeshOptions{
            .grid_lines = m_grid_lines,
            .time = 0.f,
            .color_scale = m_color_scale,
            .wire_color = {0.92f, 0.96f, 1.f, 0.32f},
            .fill_color_mode = SurfaceFillColorMode::HeightCell,
            .build_contour = true
        });
    }

    void draw_canvas_3d() {
        ImGui::SetNextWindowPos(ImVec2(330.f, 20.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(850.f, 700.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.f);
        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground;
        ImGui::Begin("Multi-Well 3D", nullptr, flags);
        ProjectedSurfaceCanvas::draw(*m_surface, m_mesh, m_vp3d, m_particles, ProjectedSurfaceCanvasOptions{
            .grid_lines = m_grid_lines,
            .color_scale = m_color_scale,
            .wire_color = {0.92f, 0.96f, 1.f, 0.32f},
            .show_frenet = m_show_frenet,
            .show_osculating_circle = m_show_osc,
            .overlay_frame_scale = 0.34f,
            .canvas_id = "multiwell_canvas",
            .help_text = "Right-drag: orbit   Scroll: zoom   Ctrl+A: avoider   Ctrl+C: centroid seeker",
            .paused = m_paused
        });
        ImGui::End();
    }

    void submit_contour_second_window() {
        rebuild_geometry_if_needed();
        submit_contour_window(m_api, m_mesh, m_particles, ContourWindowOptions{
            .extent = m_surface->extent(),
            .trail_alpha_floor = 0.70f
        });
    }

    void draw_particles_body() {
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
        if (ImGui::Button("Spawn avoider  [Ctrl+A]", ImVec2(-1.f, 0.f))) spawn_avoider();
        if (ImGui::Button("Spawn centroid seeker  [Ctrl+C]", ImVec2(-1.f, 0.f))) spawn_centroid_seeker();
        if (ImGui::Button("Reset showcase  [Ctrl+R]", ImVec2(-1.f, 0.f))) spawn_showcase_service();
        if (ImGui::Button("Clear all", ImVec2(-1.f, 0.f))) {
            m_particles.clear();
            m_particles.clear_goals();
            m_goal_status = GoalStatus::Running;
        }
        if (m_goal_status == GoalStatus::Succeeded)
            ImGui::TextColored({0.4f, 1.f, 0.4f, 1.f}, "Capture reached - paused");
        ParticleInspectorPanel::draw(m_particles.particles(), ParticleInspectorOptions{
            .label = "Active particles",
            .show_level_curve_controls = false,
            .show_brownian_controls = true,
            .show_trail_controls = true
        });
    }

    void draw_camera_body() {
        ImGui::SliderFloat("Yaw##mw", &m_vp3d.yaw, -std::numbers::pi_v<float>, std::numbers::pi_v<float>, "%.2f");
        ImGui::SliderFloat("Pitch##mw", &m_vp3d.pitch, -1.5f, 1.5f, "%.2f");
        ImGui::SliderFloat("Zoom##mw", &m_vp3d.zoom, 0.1f, 8.f, "%.2f");
        if (ImGui::Button("Reset##mw")) m_vp3d.reset();
    }

};

} // namespace ndde
