#pragma once
// app/AnalysisScene.hpp
// AnalysisScene: differential geometry analysis on the SineRationalSurface.
//
// Surface
// ───────
//   f(x,y) = [3/(1+(x+y+1)²)] sin(2x) cos(2y) + 0.1 sin(5x) sin(5y)
//   Domain: [-4, 4]²
//
// This scene demonstrates level-curve tracing. Particles (LevelCurveWalker)
// spawn at a random position and lock onto the level curve f(x,y) = z₀ where
// z₀ is the height at the spawn point. An ε-band confinement term keeps them
// from drifting off the curve.
//
// Panel layout (each is a separate, independently moveable ImGui window)
// ─────────────────────────────────────────────────────────────────────
//   "Analysis – Surface"    surface geometry readout (K, H, grad, z at cursor)
//   "Analysis – Walkers"    spawn controls, per-walker params (z₀, ε, speed)
//   "Analysis – Camera"     yaw/pitch/zoom, reset
//   Engine-global panel owns simulation switching and frame stats.
//
// The 3D canvas ("Analysis 3D") fills the dockspace exactly as in
// SurfaceSimScene. The second window shows a 2D heat/contour view of the same
// height field so the level-curve walkers have a planar reference.

#include "engine/EngineAPI.hpp"
#include "app/SceneFactories.hpp"
#include "app/SimulationSceneBase.hpp"
#include "app/AnimatedCurve.hpp"
#include "app/ContourWindowRenderer.hpp"
#include "app/FrenetFrame.hpp"
#include "app/ParticleFactory.hpp"
#include "app/HotkeyManager.hpp"
#include "app/PanelHost.hpp"
#include "app/ParticleInspectorPanel.hpp"
#include "app/ProjectedSurfaceCanvas.hpp"
#include "app/SurfaceRegistry.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "app/Viewport.hpp"
#include "app/ParticleSystem.hpp"
#include "sim/LevelCurveWalker.hpp"
#include "sim/BrownianMotion.hpp"
#include "sim/IConstraint.hpp"
#include "math/GeometryTypes.hpp"
#include "numeric/ops.hpp"

#include <imgui.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <string>
#include <random>
#include <numbers>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace ndde {

class AnalysisScene final : public SimulationSceneBase {
public:
    explicit AnalysisScene(EngineAPI api)
        : m_api(std::move(api))
        , m_surface(SurfaceRegistry::make_sine_rational(4.f))
        , m_particles(m_surface.get())
    {
        m_vp3d.base_extent = 5.f;
        m_vp3d.zoom        = 1.f;
        m_vp3d.yaw         = 0.6f;
        m_vp3d.pitch       = 0.45f;

        // Register hotkeys
        m_hotkeys.register_action(Chord::ctrl(ImGuiKey_W), "Spawn walker",
            [this]{ spawn_walker(); }, "Walkers");
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

    // ── IScene ────────────────────────────────────────────────────────────────

    void on_frame(f32 dt) override {
        advance_particles(m_particles, dt);
        draw_dockspace_root("##analysis_dock", "AnalysisDockSpace");

        draw_canvas_3d();
        m_panels.draw_all();
        submit_contour_second_window();
        draw_hotkey_panel(m_hotkeys);
    }

    [[nodiscard]] std::string_view name() const override { return "Analysis – Sine-Rational"; }
    void on_key_event(int key, int action, int mods) override {
        if (action != GLFW_PRESS) return;
        const bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
        const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
        if (!ctrl || shift) return;
        switch (key) {
            case GLFW_KEY_W: spawn_walker(); break;
            case GLFW_KEY_P: m_paused = !m_paused; break;
            case GLFW_KEY_F: m_show_frenet = !m_show_frenet; break;
            case GLFW_KEY_O: m_show_osc = !m_show_osc; break;
            case GLFW_KEY_H: m_show_hotkeys = !m_show_hotkeys; break;
            default: break;
        }
    }

private:
    // ── Core state ────────────────────────────────────────────────────────────
    EngineAPI                                       m_api;
    std::unique_ptr<ndde::math::SineRationalSurface> m_surface;
    ParticleSystem                                  m_particles;
    HotkeyManager                                   m_hotkeys;
    PanelHost                                       m_panels;
    Viewport                                        m_vp3d;

    // Geometry cache
    SurfaceMeshCache  m_mesh;
    u32               m_grid_lines  = 60;
    float             m_curv_scale  = 3.f;
    bool              m_show_frenet = true;
    bool              m_show_osc = true;

    // Spawn params
    float m_epsilon    = 0.15f;
    float m_walk_speed = 0.7f;
    float m_noise_sigma = 0.0f;
    u32   m_spawn_count = 0;

    // ── Spawn ─────────────────────────────────────────────────────────────────

    void register_panels() {
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

    [[nodiscard]] static ndde::sim::LevelCurveWalker* level_walker(AnimatedCurve& c) noexcept {
        if (auto* eq = dynamic_cast<ndde::sim::LevelCurveWalker*>(c.equation()))
            return eq;
        if (auto* stack = dynamic_cast<BehaviorStack*>(c.equation()))
            return stack->find_equation<ndde::sim::LevelCurveWalker>();
        return nullptr;
    }

    [[nodiscard]] static const ndde::sim::LevelCurveWalker* level_walker(const AnimatedCurve& c) noexcept {
        if (const auto* eq = dynamic_cast<const ndde::sim::LevelCurveWalker*>(c.equation()))
            return eq;
        if (const auto* stack = dynamic_cast<const BehaviorStack*>(c.equation()))
            return stack->find_equation<ndde::sim::LevelCurveWalker>();
        return nullptr;
    }

    void spawn_showcase_service() {
        m_particles.clear();
        m_particles.clear_goals();
        m_spawn_count = 0;
        reset_simulation_clock();

        ndde::sim::LevelCurveWalker::Params p;
        p.z0 = m_surface->height(-1.25f, 0.65f);
        p.epsilon = 0.18f;
        p.walk_speed = 0.72f;
        p.tangent_floor = 0.42f;

        ParticleBuilder leader_builder = m_particles.factory().particle();
        leader_builder
            .named("Leader - Level Curve - Brownian")
            .role(ParticleRole::Leader)
            .at({-1.25f, 0.65f})
            .history(640, 1.f / 120.f)
            .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.012f})
            .stochastic()
            .with_equation(std::make_unique<ndde::sim::LevelCurveWalker>(p))
            .with_behavior<BrownianBehavior>(0.20f, BrownianBehavior::Params{
                .sigma = 0.045f,
                .drift_strength = 0.f
            });
        AnimatedCurve& leader = m_particles.spawn(std::move(leader_builder));
        ++m_spawn_count;

        for (int i = 0; i < 180; ++i) {
            m_sim_time += 1.f / 60.f;
            m_particles.update(1.f / 60.f, 1.f, m_sim_time);
        }

        SeekParticleBehavior::Params seek;
        seek.target = TargetSelector::nearest(ParticleRole::Leader);
        seek.speed = 0.86f;
        seek.delay_seconds = 0.85f;

        const glm::vec2 leader_uv = leader.head_uv();
        ParticleBuilder seeker_builder = m_particles.factory().particle();
        seeker_builder
            .named("Chaser - Delayed Seek - Brownian")
            .role(ParticleRole::Chaser)
            .at({leader_uv.x + 1.0f, leader_uv.y - 0.75f})
            .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.012f})
            .stochastic()
            .with_behavior<SeekParticleBehavior>(seek)
            .with_behavior<BrownianBehavior>(0.18f, BrownianBehavior::Params{
                .sigma = 0.035f,
                .drift_strength = 0.f
            });
        m_particles.spawn(std::move(seeker_builder));
        ++m_spawn_count;

        m_particles.add_goal<CaptureGoal>(CaptureGoal::Params{
            .seeker_role = ParticleRole::Chaser,
            .target_role = ParticleRole::Leader,
            .radius = 0.18f
        });
        m_goal_status = GoalStatus::Running;
    }

    void spawn_walker() {
        // Pick a random position in the domain
        std::uniform_real_distribution<float> du(
            m_surface->u_min() + 0.5f, m_surface->u_max() - 0.5f);
        std::uniform_real_distribution<float> dv(
            m_surface->v_min() + 0.5f, m_surface->v_max() - 0.5f);

        const float u0 = du(m_particles.rng());
        const float v0 = dv(m_particles.rng());
        const float z0 = m_surface->height(u0, v0);

        ndde::sim::LevelCurveWalker::Params p;
        p.z0         = z0;
        p.epsilon    = m_epsilon;
        p.walk_speed = m_walk_speed;

        ParticleBuilder builder = m_particles.factory().particle();
        builder.named("Walker")
            .role(ParticleRole::Leader)
            .at({u0, v0})
            .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
            .with_equation(std::make_unique<ndde::sim::LevelCurveWalker>(p));

        if (m_noise_sigma > 1e-6f) {
            builder.stochastic()
                .with_behavior<BrownianBehavior>(BrownianBehavior::Params{
                    .sigma = m_noise_sigma,
                    .drift_strength = 0.f
                });
        }

        AnimatedCurve& c = m_particles.spawn(std::move(builder));

        SimulationContext context = m_particles.context(m_sim_time);
        context.set_time(m_sim_time);
        c.set_behavior_context(&context);

        // Pre-warm 120 frames so trail is visible immediately
        for (int i = 0; i < 120; ++i) {
            context.set_time(m_sim_time + static_cast<float>(i) / 60.f);
            c.advance(1.f/60.f, m_sim_speed);
        }
        ++m_spawn_count;
    }

    // ── Surface geometry ──────────────────────────────────────────────────────

    void rebuild_geometry_if_needed() {
        m_mesh.rebuild_if_needed(*m_surface, SurfaceMeshOptions{
            .grid_lines = m_grid_lines,
            .time = 0.f,
            .color_scale = m_curv_scale,
            .wire_color = {0.3f, 0.6f, 0.9f, 0.55f},
            .fill_color_mode = SurfaceFillColorMode::HeightCell,
            .build_contour = true
        });
    }

    void submit_contour_second_window() {
        rebuild_geometry_if_needed();
        submit_contour_window(m_api, m_mesh, m_particles, ContourWindowOptions{
            .extent = m_surface->extent(),
            .draw_wire = true,
            .wire_color = {0.95f, 0.95f, 1.f, 0.32f},
            .trail_alpha_floor = 0.65f,
            .draw_heads = true
        });
    }

    // ── 3D canvas window ──────────────────────────────────────────────────────

    void draw_canvas_3d() {
        ImGui::SetNextWindowPos(ImVec2(330.f, 20.f),  ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(750.f, 660.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.f);
        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoBackground;
        ImGui::Begin("Analysis 3D", nullptr, flags);

        ProjectedSurfaceCanvas::draw(*m_surface, m_mesh, m_vp3d, m_particles, ProjectedSurfaceCanvasOptions{
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
        });

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 cpos = ImGui::GetItemRectMin();
        const ImVec2 csz = ImGui::GetItemRectSize();

        // Level-band indicator lines per walker
        for (const auto& c : m_particles.particles()) {
            if (!c.has_trail()) continue;
            if (const auto* eq = level_walker(c)) {
                const float z0  = eq->params().z0;
                const float eps = eq->params().epsilon;
                // Draw small dot at head coloured by how close we are to z0
                const Vec3 hp = c.head_world();
                const ImVec2 pp = ProjectedSurfaceCanvas::projected_to_canvas(
                    ProjectedSurfaceCanvas::project_point(hp, m_vp3d), cpos, csz, m_surface->extent());
                const float dz = std::abs(hp.z - z0);
                const float t  = std::clamp(dz / (eps + 1e-6f), 0.f, 1.f);
                dl->AddCircleFilled(pp, 3.5f,
                    IM_COL32(static_cast<int>((1.f - t) * 255.f),
                             static_cast<int>((0.3f + 0.5f * (1.f - t)) * 255.f),
                             static_cast<int>(t * 0.8f * 255.f),
                             255),
                    14);
            }
        }

        ImGui::End();
    }

    // ── Panels ────────────────────────────────────────────────────────────────

    void draw_surface_body() {
        ImGui::SeparatorText("Function");
        ImGui::TextDisabled("f(x,y) = [3/(1+(x+y+1)\xc2\xb2)] sin(2x)cos(2y)");
        ImGui::TextDisabled("       + 0.1 sin(5x) sin(5y)");
        ImGui::Spacing();

        ImGui::SeparatorText("Geometry");
        {
            int gl = static_cast<int>(m_grid_lines);
            if (ImGui::SliderInt("Grid lines##as", &gl, 8, 120)) {
                m_grid_lines = static_cast<u32>(gl);
                m_mesh.mark_dirty();
            }
            if (ImGui::SliderFloat("Color scale##as", &m_curv_scale, 0.1f, 10.f, "%.2f"))
                m_mesh.mark_dirty();
            ImGui::TextDisabled("~%u k verts", (4u*m_grid_lines*(m_grid_lines+1u))/1000u);
            ImGui::Checkbox("Hover Frenet  [Ctrl+F]", &m_show_frenet);
            ImGui::Checkbox("Osculating circle  [Ctrl+O]", &m_show_osc);
        }

        ImGui::SeparatorText("At cursor (u,v)");
        // Show surface values at arbitrary position entered by user
        {
            static float probe_u = 0.f, probe_v = 0.f;
            ImGui::SetNextItemWidth(90.f);
            ImGui::InputFloat("u##probe", &probe_u, 0.1f, 0.5f, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90.f);
            ImGui::InputFloat("v##probe", &probe_v, 0.1f, 0.5f, "%.2f");
            probe_u = std::clamp(probe_u, m_surface->u_min(), m_surface->u_max());
            probe_v = std::clamp(probe_v, m_surface->v_min(), m_surface->v_max());

            const float z  = m_surface->height(probe_u, probe_v);
            const float gx = m_surface->du(probe_u, probe_v).z;
            const float gy = m_surface->dv(probe_u, probe_v).z;
            const float gm = ops::sqrt(gx*gx + gy*gy);
            const float K  = m_surface->gaussian_curvature(probe_u, probe_v);
            const float H  = m_surface->mean_curvature(probe_u, probe_v);

            ImGui::TextDisabled("z     = %.4f", z);
            ImGui::TextDisabled("|grad| = %.4f", gm);
            ImGui::TextDisabled("grad  = (%.3f, %.3f)", gx, gy);
            ImGui::TextDisabled("K     = %.5f", K);
            ImGui::TextDisabled("H     = %.5f", H);
            const float perpx = -gy / (gm + 1e-9f);
            const float perpy =  gx / (gm + 1e-9f);
            ImGui::TextDisabled("level-tangent = (%.3f, %.3f)", perpx, perpy);
        }
    }

    void draw_walkers_body() {
        ImGui::SeparatorText("Spawn parameters");
        ImGui::SliderFloat("Epsilon (band)##w",  &m_epsilon,    0.02f, 1.f,  "%.3f");
        ImGui::SliderFloat("Walk speed##w",       &m_walk_speed, 0.1f,  2.f,  "%.2f");
        ImGui::SliderFloat("Noise sigma##w",      &m_noise_sigma,0.f,   0.5f, "%.3f");
        ImGui::SliderFloat("Sim speed##w",        &m_sim_speed,  0.1f,  5.f,  "%.2f");
        ImGui::Spacing();

        if (ImGui::Button("Spawn walker  [Ctrl+W]", ImVec2(-1.f, 0.f)))
            spawn_walker();
        if (ImGui::Button("Reset showcase", ImVec2(-1.f, 0.f)))
            spawn_showcase_service();
        if (ImGui::Button("Clear all", ImVec2(-1.f, 0.f))) {
            m_particles.clear();
            m_particles.clear_goals();
            m_spawn_count = 0;
            m_goal_status = GoalStatus::Running;
        }

        if (m_goal_status == GoalStatus::Succeeded)
            ImGui::TextColored({0.4f, 1.f, 0.4f, 1.f}, "Capture reached - paused");
        ParticleInspectorPanel::draw(m_particles.particles(), ParticleInspectorOptions{
            .label = "Active walkers",
            .show_level_curve_controls = true,
            .show_brownian_controls = true,
            .show_trail_controls = true
        });
    }

    void draw_camera_body() {
        ImGui::SliderFloat("Yaw##ac",   &m_vp3d.yaw,
                           -std::numbers::pi_v<float>, std::numbers::pi_v<float>, "%.2f");
        ImGui::SliderFloat("Pitch##ac", &m_vp3d.pitch, -1.5f, 1.5f, "%.2f");
        ImGui::SliderFloat("Zoom##ac",  &m_vp3d.zoom,   0.1f,  8.f, "%.2f");
        if (ImGui::Button("Reset##ac")) m_vp3d.reset();

        ImGui::SeparatorText("Pause");
        ImGui::Checkbox("Paused  [Ctrl+P]", &m_paused);
    }

};

} // namespace ndde
