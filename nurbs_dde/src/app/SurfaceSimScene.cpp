// app/SurfaceSimScene.cpp
// B2 refactor: submit_arrow, submit_trail_3d, submit_head_dot_3d,
// submit_frenet_3d, submit_osc_circle_3d, submit_surface_frame_3d,
// submit_normal_plane_3d, and submit_torsion_3d have been moved to
// src/app/ParticleRenderer.cpp.  This file now delegates particle
// rendering to m_particle_renderer.submit_all().
#include "app/SurfaceSimScene.hpp"
#include "app/SceneFactories.hpp"
#include "app/ParticleBehaviors.hpp"
#include "numeric/ops.hpp"
#include "sim/GradientWalker.hpp"
#include <imgui.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstring>
#include <numbers>
#include <format>
#include <algorithm>
#include <fstream>
#include <iomanip>

namespace ndde {

// ── Constructor ───────────────────────────────────────────────────────────────

SurfaceSimScene::SurfaceSimScene(EngineAPI api)
    : m_api(std::move(api))
    , m_surface(std::make_unique<GaussianSurface>())
    , m_particles(m_surface.get())
    , m_curves(m_particles.particles())
    , m_particle_renderer(m_api)  // B2: owns a copy of EngineAPI (value type)
{
    m_vp3d.base_extent = 6.f;
    m_vp3d.zoom        = 1.0f;
    m_vp3d.yaw         = 0.55f;
    m_vp3d.pitch       = 0.42f;

    m_vp2d.base_extent = 6.f;
    m_vp2d.zoom        = 1.f;
    m_vp2d.pan_x       = 0.f;
    m_vp2d.pan_y       = 0.f;

    register_panels();
    spawn_showcase_service();

    // ── Register hotkeys ──────────────────────────────────────────────────────
    // Each registration stores its own edge-detection state internally.
    // Groups appear as section headers in draw_hotkey_panel().

    // Overlays
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_F), "Frenet frame  (T, N, B)",
                               m_show_frenet,       "Overlays");
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_D), "Surface frame  (Dx, Dy)",
                               m_show_dir_deriv,    "Overlays");
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_N), "Normal plane patch",
                               m_show_normal_plane, "Overlays");
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_T), "Torsion ribbon",
                               m_show_torsion,      "Overlays");

    // Simulation
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_P), "Pause / unpause",
        [this]{ m_sim_paused = !m_sim_paused; }, "Simulation");

    // Spawn
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_L), "Leader particle  (blue)",
        [this]{
            spawn_gradient_particle(ParticleRole::Leader,
                offset_spawn(reference_uv(), 1.5f, static_cast<float>(m_leader_count) * 1.1f));
        }, "Spawn");

    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_C), "Chaser particle  (red)",
        [this]{
            spawn_gradient_particle(ParticleRole::Chaser,
                offset_spawn(reference_uv(), 2.0f, static_cast<float>(m_chaser_count) * 1.3f + 0.5f));
        }, "Spawn");

    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_B), "Brownian particle  (Milstein)",
        [this]{
            spawn_brownian_particle(offset_spawn(reference_uv(), 1.8f,
                static_cast<float>(m_chaser_count + m_leader_count) * 0.7f + 1.0f));
        }, "Spawn");

    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_R), "Delay-pursuit chaser",
        [this]{
            if (m_curves.empty()) return;
            if (m_curves[0].history() == nullptr) {
                const std::size_t cap =
                    static_cast<std::size_t>(std::ceil(m_dp_params.tau * 120.f * 1.5f)) + 256;
                m_curves[0].enable_history(cap, 1.f / 120.f);
            }
            const glm::vec2 ref = m_curves[0].head_uv();
            spawn_delay_pursuit_particle(offset_spawn(ref, 2.0f,
                static_cast<float>(m_dp_count) * 1.1f + 0.3f));
        }, "Spawn");

    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_A), "Leader seeker / pursuer  [Ctrl+A]",
        [this]{
            if (!m_spawning_pursuer) spawn_leader_seeker();
            else                     spawn_pursuit_particle();
        }, "Spawn");

    // Panels
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_Q), "Coordinate debug panel",
        [this]{
            m_debug_open = !m_debug_open;
            m_coord_debug.visible() = m_debug_open;
        }, "Panels");

    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_H), "This hotkey panel",
                               m_hotkey_panel_open, "Panels");
}

// ── on_frame ──────────────────────────────────────────────────────────────────

void SurfaceSimScene::on_frame(f32 dt) {
    advance_simulation(dt);
    m_goal_status = m_particles.evaluate_goals(m_sim_time);
    if (m_goal_status == GoalStatus::Succeeded)
        m_sim_paused = true;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    constexpr ImGuiWindowFlags dock_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
    ImGui::Begin("##dockspace_root", nullptr, dock_flags);
    ImGui::PopStyleVar(3);
    const ImGuiID dock_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dock_id, ImVec2(0.f, 0.f), ImGuiDockNodeFlags_None);
    ImGui::End();

    m_panels.draw_all();
    draw_surface_3d_window();
    submit_contour_second_window();
    draw_hotkey_panel();

    struct StubConic { std::string name; std::vector<int> snap_cache; };
    const std::vector<StubConic> no_conics;
    m_coord_debug.update(m_vp3d, m_hover, no_conics, m_api.viewport_size());
    m_coord_debug.draw();
    m_debug_open = m_coord_debug.visible();
}

void SurfaceSimScene::on_key_event(int key, int action, int mods) {
    if (action != GLFW_PRESS) return;
    const bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    if (!ctrl || shift) return;

    switch (key) {
        case GLFW_KEY_F: m_show_frenet = !m_show_frenet; break;
        case GLFW_KEY_D: m_show_dir_deriv = !m_show_dir_deriv; break;
        case GLFW_KEY_N: m_show_normal_plane = !m_show_normal_plane; break;
        case GLFW_KEY_T: m_show_torsion = !m_show_torsion; break;
        case GLFW_KEY_P: m_sim_paused = !m_sim_paused; break;
        case GLFW_KEY_H: m_hotkey_panel_open = !m_hotkey_panel_open; break;
        case GLFW_KEY_Q:
            m_debug_open = !m_debug_open;
            m_coord_debug.visible() = m_debug_open;
            break;
        case GLFW_KEY_L: {
            spawn_gradient_particle(ParticleRole::Leader,
                offset_spawn(reference_uv(), 1.5f, static_cast<float>(m_leader_count) * 1.1f));
            break;
        }
        case GLFW_KEY_C: {
            spawn_gradient_particle(ParticleRole::Chaser,
                offset_spawn(reference_uv(), 2.0f, static_cast<float>(m_chaser_count) * 1.3f + 0.5f));
            break;
        }
        case GLFW_KEY_B: {
            spawn_brownian_particle(offset_spawn(reference_uv(), 1.8f,
                static_cast<float>(m_chaser_count + m_leader_count) * 0.7f + 1.0f));
            break;
        }
        case GLFW_KEY_R: {
            if (m_curves.empty()) break;
            if (m_curves[0].history() == nullptr) {
                const std::size_t cap =
                    static_cast<std::size_t>(std::ceil(m_dp_params.tau * 120.f * 1.5f)) + 256;
                m_curves[0].enable_history(cap, 1.f / 120.f);
            }
            const glm::vec2 ref = m_curves[0].head_uv();
            spawn_delay_pursuit_particle(offset_spawn(ref, 2.0f,
                static_cast<float>(m_dp_count) * 1.1f + 0.3f));
            break;
        }
        case GLFW_KEY_A:
            if (!m_spawning_pursuer) spawn_leader_seeker();
            else                     spawn_pursuit_particle();
            break;
        default:
            break;
    }
}

// ── swap_surface ──────────────────────────────────────────────────────────────

void SurfaceSimScene::swap_surface(SurfaceType type) {
    switch (type) {
        case SurfaceType::Gaussian:
            m_surface = std::make_unique<GaussianSurface>();
            break;
        case SurfaceType::Torus:
            m_surface = std::make_unique<ndde::math::Torus>(m_torus_R, m_torus_r);
            break;
        case SurfaceType::GaussianRipple: {
            auto s = std::make_unique<GaussianRipple>(m_ripple_params);
            m_surface = std::move(s);
            break;
        }
        case SurfaceType::Extremum:
            m_surface = std::make_unique<ndde::math::ExtremumSurface>();
            m_extremum_table.build(*m_surface);
            break;
    }
    m_surface_type    = type;
    m_particles.set_surface(m_surface.get());
    m_sim_time        = 0.f;
    m_mesh.mark_dirty();
    m_spawning_pursuer = false;

    std::vector<ParticleRole> roles;
    roles.reserve(m_curves.size());
    for (const auto& curve : m_curves)
        roles.push_back(curve.particle_role());
    m_particles.clear();
    m_leader_count = 0;
    m_chaser_count = 0;

    const u32 n  = static_cast<u32>(roles.size());
    const f32 u0 = m_surface->u_min(), u1 = m_surface->u_max();
    const f32 v0 = m_surface->v_min(), v1 = m_surface->v_max();
    const f32 um = m_surface->is_periodic_u() ? 0.f : (u1-u0)*0.08f;
    const f32 vm = m_surface->is_periodic_v() ? 0.f : (v1-v0)*0.08f;
    const f32 u_lo = u0+um, u_hi = u1-um;
    const f32 v_lo = v0+vm, v_hi = v1-vm;

    for (u32 i = 0; i < n; ++i) {
        const f32 su = (static_cast<f32>(i)+0.5f)/static_cast<f32>(std::max(n,1u));
        const f32 sv = std::fmod(su * 2.618033988f, 1.f);
        spawn_gradient_particle(roles[i], {u_lo + su*(u_hi-u_lo), v_lo + sv*(v_hi-v_lo)});
        prewarm_particle(m_curves.back(), 60);
    }
}

// ── advance_simulation ────────────────────────────────────────────────────────

void SurfaceSimScene::advance_simulation(f32 dt) {
    if (!m_sim_paused) {
        if (auto* def = dynamic_cast<ndde::math::IDeformableSurface*>(m_surface.get()))
            def->advance(dt * m_sim_speed);
        m_sim_time += dt * m_sim_speed;

        rebuild_extremum_table_if_needed();

        m_particles.update(dt, m_sim_speed, m_sim_time);

        // Apply pairwise constraints (e.g. min-distance collision avoidance)
        if (!m_pair_constraints.empty())
            apply_pairwise_constraints();
    }
}

// ── apply_pairwise_constraints ────────────────────────────────────────────────

void SurfaceSimScene::apply_pairwise_constraints() {
    const std::size_t n = m_curves.size();
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            for (const auto& pc : m_pair_constraints)
                pc->apply(m_curves[i].walk_state(),
                           m_curves[j].walk_state(),
                           *m_surface);
        }
    }
}

glm::vec2 SurfaceSimScene::reference_uv() const noexcept {
    if (m_curves.empty())
        return {(m_surface->u_min() + m_surface->u_max()) * 0.5f,
                (m_surface->v_min() + m_surface->v_max()) * 0.5f};
    return m_curves[0].head_uv();
}

glm::vec2 SurfaceSimScene::offset_spawn(glm::vec2 ref_uv, float radius, float angle) const noexcept {
    constexpr float margin = 0.5f;
    return {
        std::clamp(ref_uv.x + radius * ops::cos(angle),
                   m_surface->u_min() + margin, m_surface->u_max() - margin),
        std::clamp(ref_uv.y + radius * ops::sin(angle),
                   m_surface->v_min() + margin, m_surface->v_max() - margin)
    };
}

void SurfaceSimScene::prewarm_particle(AnimatedCurve& particle, int frames) {
    SimulationContext context = m_particles.context(m_sim_time);
    particle.set_behavior_context(&context);
    for (int i = 0; i < frames; ++i) {
        context.set_time(m_sim_time + static_cast<float>(i) / 60.f);
        particle.advance(1.f / 60.f, m_sim_speed);
    }
    particle.set_behavior_context(nullptr);
}

void SurfaceSimScene::spawn_gradient_particle(ParticleRole role, glm::vec2 uv) {
    ParticleBuilder builder = m_particles.factory().particle();
    builder
        .named(std::string(role_name(role)) + " - Gradient Walker")
        .role(role)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .with_equation(std::make_unique<ndde::sim::GradientWalker>());

    AnimatedCurve& particle = m_particles.spawn(std::move(builder));
    prewarm_particle(particle);

    if (role == ParticleRole::Leader) ++m_leader_count;
    else if (role == ParticleRole::Chaser) ++m_chaser_count;
}

void SurfaceSimScene::spawn_brownian_particle(glm::vec2 uv) {
    ParticleBuilder builder = m_particles.factory().particle();
    builder
        .named("Chaser - Brownian")
        .role(ParticleRole::Chaser)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_equation(std::make_unique<ndde::sim::BrownianMotion>(m_bm_params));

    AnimatedCurve& particle = m_particles.spawn(std::move(builder));
    prewarm_particle(particle);
    ++m_chaser_count;
}

void SurfaceSimScene::spawn_delay_pursuit_particle(glm::vec2 uv) {
    if (m_curves.empty()) return;
    if (m_curves[0].history() == nullptr) {
        const std::size_t cap =
            static_cast<std::size_t>(std::ceil(m_dp_params.tau * 120.f * 1.5f)) + 256;
        m_curves[0].enable_history(cap, 1.f / 120.f);
    }

    ParticleBuilder builder = m_particles.factory().particle();
    builder
        .named("Chaser - Delay Pursuit")
        .role(ParticleRole::Chaser)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_equation(std::make_unique<ndde::sim::DelayPursuitEquation>(
            m_curves[0].history(), m_surface.get(), m_dp_params));
    m_particles.spawn(std::move(builder));
    ++m_chaser_count;
    ++m_dp_count;
}

void SurfaceSimScene::spawn_showcase_service() {
    m_curves.clear();
    m_particles.clear_goals();
    m_leader_count = 0;
    m_chaser_count = 0;
    m_dp_count = 0;
    m_spawning_pursuer = true;

    const std::size_t history_cap =
        static_cast<std::size_t>(std::ceil(2.0f * 120.f * 1.5f)) + 256;

    ParticleBuilder leader_builder = m_particles.factory().particle();
    leader_builder
        .named("Leader - Brownian - Bias Drift")
        .role(ParticleRole::Leader)
        .at({-4.5f, -4.0f})
        .history(history_cap, 1.f / 120.f)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_behavior<ConstantDriftBehavior>(0.75f, glm::vec2{0.55f, 0.28f})
        .with_behavior<BrownianBehavior>(BrownianBehavior::Params{
            .sigma = 0.16f,
            .drift_strength = 0.08f
        });
    AnimatedCurve& leader = m_particles.spawn(std::move(leader_builder));
    ++m_leader_count;

    for (int i = 0; i < 240; ++i) {
        m_sim_time += 1.f / 60.f;
        m_particles.update(1.f / 60.f, 1.f, m_sim_time);
    }

    ndde::SeekParticleBehavior::Params seek;
    seek.target = TargetSelector::nearest(ParticleRole::Leader);
    seek.speed = 0.95f;
    seek.delay_seconds = 1.0f;

    const glm::vec2 uv = offset_spawn(leader.head_uv(), 2.3f, 0.4f);
    ParticleBuilder seeker_builder = m_particles.factory().particle();
    seeker_builder
        .named("Chaser - Delayed Seek - Brownian")
        .role(ParticleRole::Chaser)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_behavior<SeekParticleBehavior>(seek)
        .with_behavior<BrownianBehavior>(0.25f, BrownianBehavior::Params{
            .sigma = 0.06f,
            .drift_strength = 0.f
        });
    m_particles.spawn(std::move(seeker_builder));
    ++m_chaser_count;
    ++m_dp_count;

    m_particles.add_goal<CaptureGoal>(CaptureGoal::Params{
        .seeker_role = ParticleRole::Chaser,
        .target_role = ParticleRole::Leader,
        .radius = 0.22f
    });
    m_goal_status = GoalStatus::Running;
}

// ── handle_hotkeys ────────────────────────────────────────────────────────────
// Removed: hotkey dispatch is now owned by m_hotkeys (HotkeyManager).
// All registrations happen in the constructor. Call m_hotkeys.dispatch() per frame.

// ── canvas_mvp_3d ─────────────────────────────────────────────────────────────

Mat4 SurfaceSimScene::canvas_mvp_3d(const ImVec2& cpos, const ImVec2& csz) const noexcept {
    const Vec2 sw_sz = m_api.viewport_size();
    const f32  sw    = sw_sz.x > 0.f ? sw_sz.x : 1.f;
    const f32  sh    = sw_sz.y > 0.f ? sw_sz.y : 1.f;
    const f32  cw    = csz.x   > 0.f ? csz.x   : 1.f;
    const f32  ch    = csz.y   > 0.f ? csz.y   : 1.f;
    const ImVec2 vp0 = ImGui::GetMainViewport()->Pos;
    const f32  cx    = cpos.x - vp0.x;
    const f32  cy    = cpos.y - vp0.y;

    const f32 sx = cw / sw;
    const f32 sy = ch / sh;
    const f32 bx =  2.f*cx/sw + sx - 1.f;
    const f32 by = -(2.f*cy/sh + sy - 1.f);

    Mat4 remap(0.f);
    remap[0][0] = sx;  remap[1][1] = sy;  remap[2][2] = 1.f;
    remap[3][0] = bx;  remap[3][1] = by;  remap[3][3] = 1.f;

    const f32  aspect = cw / ch;
    const f32  dist   = m_vp3d.base_extent / m_vp3d.zoom * 3.f;
    const f32  ex     = m_vp3d.pan_x + dist*ops::cos(m_vp3d.pitch)*ops::sin(m_vp3d.yaw);
    const f32  ey     = m_vp3d.pan_y + dist*ops::sin(m_vp3d.pitch);
    const f32  ez     =                dist*ops::cos(m_vp3d.pitch)*ops::cos(m_vp3d.yaw);
    const Mat4 proj   = glm::perspective(glm::radians(45.f), aspect, 0.01f, 500.f);
    const Mat4 view   = glm::lookAt(
        glm::vec3(ex, ey, ez),
        glm::vec3(m_vp3d.pan_x, m_vp3d.pan_y, 0.f),
        glm::vec3(0.f, 1.f, 0.f));
    return remap * proj * view;
}

// ── draw_hotkey_panel ─────────────────────────────────────────────────────────

void SurfaceSimScene::register_panels() {
    m_panels.clear();
    m_panels.add({
        .title = "Sim - Surface",
        .default_pos = {20.f, 20.f},
        .default_size = {300.f, 220.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_surface(); }
    });
    m_panels.add({
        .title = "Sim - Particles",
        .default_pos = {20.f, 250.f},
        .default_size = {300.f, 180.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_particles(); }
    });
    m_panels.add({
        .title = "Sim - Overlays",
        .default_pos = {20.f, 440.f},
        .default_size = {300.f, 240.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_overlays(); }
    });
    m_panels.add({
        .title = "Sim - Brownian  [Ctrl+B]",
        .default_pos = {20.f, 690.f},
        .default_size = {300.f, 260.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_brownian(); }
    });
    m_panels.add({
        .title = "Sim - Pursuit",
        .default_pos = {330.f, 700.f},
        .default_size = {310.f, 320.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_pursuit(); }
    });
    m_panels.add({
        .title = "Sim - Geometry",
        .default_pos = {650.f, 700.f},
        .default_size = {310.f, 280.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_geometry(); }
    });
    m_panels.add({
        .title = "Sim - Camera",
        .default_pos = {330.f, 20.f},
        .default_size = {310.f, 170.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_camera(); }
    });
}

void SurfaceSimScene::draw_hotkey_panel() {
    // Delegate entirely to HotkeyManager. The panel lists every registered
    // binding in group order; no hardcoded row table needed here.
    m_hotkeys.draw_panel("Hotkeys  [Ctrl+H]", m_hotkey_panel_open);
}

// ── draw_panel_surface ────────────────────────────────────────────────────────

void SurfaceSimScene::draw_panel_surface() {
    ImGui::SeparatorText("Surface type");
    {
        int sel = static_cast<int>(m_surface_type);
        bool changed = false;
        changed |= ImGui::RadioButton("Gaussian",       &sel, 0);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Torus",          &sel, 1);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Ripple",         &sel, 2);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Extremum##surf", &sel, 3);
        if (changed) swap_surface(static_cast<SurfaceType>(sel));

        if (m_surface_type == SurfaceType::Torus) {
            bool tp = false;
            tp |= ImGui::SliderFloat("R##torus", &m_torus_R, 0.5f, 5.f, "%.2f");
            tp |= ImGui::SliderFloat("r##torus", &m_torus_r, 0.1f, m_torus_R - 0.05f, "%.2f");
            if (tp) swap_surface(SurfaceType::Torus);
        }

        if (m_surface_type == SurfaceType::GaussianRipple) {
            auto& p  = m_ripple_params;
            bool  rp = false;
            rp |= ImGui::SliderFloat("Amplitude##r",  &p.amplitude,  0.05f, 2.f,  "%.2f");
            rp |= ImGui::SliderFloat("Damping##r",    &p.damping,    0.05f, 2.f,  "%.3f");
            rp |= ImGui::SliderFloat("Wavelength##r", &p.wavelength, 0.3f,  5.f,  "%.2f");
            rp |= ImGui::SliderFloat("Speed##r",      &p.speed,      0.1f,  6.f,  "%.2f");
            rp |= ImGui::SliderFloat("Sigma##r",      &p.sigma,      0.3f,  6.f,  "%.2f");
            if (rp) {
                if (auto* gr = dynamic_cast<GaussianRipple*>(m_surface.get()))
                    gr->params() = p;
            }
            ImGui::TextDisabled("Epicentre: (%.2f, %.2f)  t=%.2f",
                p.epicentre_u, p.epicentre_v, m_sim_time);
            ImGui::SameLine();
            if (ImGui::SmallButton("Re-trigger")) {
                const Vec3 h = m_curves.empty() ? Vec3{0,0,0} : m_curves[0].head_world();
                p.epicentre_u = h.x;  p.epicentre_v = h.y;
                if (auto* gr = dynamic_cast<GaussianRipple*>(m_surface.get()))
                    gr->set_epicentre(h.x, h.y);
                m_sim_time = 0.f;
            }
        }
    }

    ImGui::SeparatorText("Display");
    {
        int mode = static_cast<int>(m_surface_display);
        bool changed = false;
        changed |= ImGui::RadioButton("Wireframe", &mode, 0);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Filled",    &mode, 1);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Both",      &mode, 2);
        if (changed) {
            m_surface_display = static_cast<SurfaceDisplay>(mode);
            m_mesh.mark_dirty();
        }
        if (m_surface_display != SurfaceDisplay::Wireframe) {
            if (ImGui::SliderFloat("K scale##curv", &m_curv_scale, 0.01f, 20.f, "%.2f"))
                m_mesh.mark_dirty();
        }
        {
            int gl = static_cast<int>(m_grid_lines);
            if (ImGui::SliderInt("Grid lines##surface", &gl, 8, 256)) {
                m_grid_lines = static_cast<u32>(std::max(gl, 8));
                m_mesh.mark_dirty();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("~%uk verts", (4u * m_grid_lines * (m_grid_lines + 1u)) / 1000u);
        }
    }
}

// ── draw_panel_particles ──────────────────────────────────────────────────────

void SurfaceSimScene::draw_panel_particles() {
    ImGui::Checkbox("Paused  [Ctrl+P]", &m_sim_paused);
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        m_sim_time = 0.f;
        spawn_showcase_service();
    }
    ImGui::SliderFloat("Speed##sim",  &m_sim_speed,   0.1f, 5.f,  "%.2f");
    ImGui::SliderFloat("Arrow scale", &m_frame_scale, 0.05f,0.8f, "%.2f");
    ImGui::Checkbox("Contour lines",  &m_show_contours);

    ImGui::Separator();
    ImGui::TextDisabled("%zu particles  (L=%u  C=%u)",
        m_curves.size(), m_leader_count, m_chaser_count);
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear all")) {
        m_sim_time = 0.f;
        spawn_showcase_service();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Export CSV")) {
        const std::string path = "session_" +
            std::to_string(m_curves.size()) + "p_" +
            std::to_string(static_cast<int>(m_sim_time)) + "s.csv";
        export_session(path);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Write trail + history data to CSV in working directory");

    ImGui::TextDisabled("Ctrl+L leader  Ctrl+C chaser  Ctrl+B brownian");
    if (m_goal_status == GoalStatus::Succeeded)
        ImGui::TextColored({0.4f, 1.f, 0.4f, 1.f}, "Capture reached - paused");
    ParticleInspectorPanel::draw(m_particles.particles(), ParticleInspectorOptions{
        .label = "Active particles",
        .show_level_curve_controls = false,
        .show_brownian_controls = true,
        .show_trail_controls = true
    });
}

// ── draw_panel_overlays ───────────────────────────────────────────────────────

void SurfaceSimScene::draw_panel_overlays() {
    ImGui::SeparatorText("Frenet frame  [Ctrl+F]");
    ImGui::Checkbox("Show Frenet", &m_show_frenet);
    ImGui::BeginDisabled(!m_show_frenet);
    ImGui::SameLine(); ImGui::Checkbox("T##ft", &m_show_T);
    ImGui::SameLine(); ImGui::Checkbox("N##fn", &m_show_N);
    ImGui::SameLine(); ImGui::Checkbox("B##fb", &m_show_B);
    ImGui::Checkbox("Osculating circle", &m_show_osc);
    ImGui::EndDisabled();

    ImGui::SeparatorText("Other overlays");
    ImGui::Checkbox("Surface frame  [Ctrl+D]", &m_show_dir_deriv);
    ImGui::Checkbox("Normal plane   [Ctrl+N]", &m_show_normal_plane);
    ImGui::Checkbox("Torsion ribbon [Ctrl+T]", &m_show_torsion);

    // Torsion live readout when active
    if (m_show_torsion && !m_curves.empty() && m_curves[0].has_trail()) {
        const AnimatedCurve& c0 = m_curves[0];
        const u32 hi = c0.trail_size() > 2 ? c0.trail_size()-2 : 0;
        const FrenetFrame fr = c0.frenet_at(hi);
        const f32 tau = fr.tau;
        const ImVec4 tau_col =
            tau >  1e-4f ? ImVec4(1.f,  0.55f, 0.1f, 1.f)
          : tau < -1e-4f ? ImVec4(0.45f,0.8f,  1.f,  1.f)
                         : ImVec4(0.7f, 0.7f,  0.7f, 1.f);
        ImGui::Indent(8.f);
        ImGui::TextColored(tau_col, "\xcf\x84 = %+.6f", tau);
        ImGui::SameLine();
        if      (tau >  1e-4f) ImGui::TextDisabled("(right twist)");
        else if (tau < -1e-4f) ImGui::TextDisabled("(left twist)");
        else                   ImGui::TextDisabled("(planar)");
        if (fr.kappa > 1e-5f)
            ImGui::TextDisabled("|\xcf\x84/\xce\xba| = %.4f", std::abs(tau) / fr.kappa);
        ImGui::Unindent(8.f);
    }
}

// ── draw_panel_brownian ───────────────────────────────────────────────────────

void SurfaceSimScene::draw_panel_brownian() {
    auto& p = m_bm_params;
    ImGui::SliderFloat("Sigma##bm", &p.sigma,          0.01f, 2.f, "%.3f");
    ImGui::SliderFloat("Drift##bm", &p.drift_strength, -1.f,  1.f, "%.3f");
    if (ImGui::Button("Spawn Brownian  [Ctrl+B]", ImVec2(-1.f, 0.f))) {
        spawn_brownian_particle(offset_spawn(reference_uv(), 1.8f,
            static_cast<float>(m_chaser_count + m_leader_count) * 0.7f + 1.0f));
    }
    ImGui::TextDisabled("Milstein integrator (strong order 1.0)");

    ImGui::SeparatorText("RNG reproducibility");
    {
        static int seed_input = 0;
        ImGui::SetNextItemWidth(140.f);
        ImGui::InputInt("Seed (0=random)##rng", &seed_input);
        ImGui::SameLine();
        if (ImGui::SmallButton("Apply##rng"))
            ndde::sim::MilsteinIntegrator::set_global_seed(
                static_cast<uint64_t>(seed_input));
        if (ndde::sim::MilsteinIntegrator::seed_is_fixed())
            ImGui::TextColored({0.4f,1.f,0.4f,1.f}, "Seed fixed: %llu",
                (unsigned long long)ndde::sim::MilsteinIntegrator::global_seed());
        else
            ImGui::TextDisabled("Seed: random (hardware)");
        ImGui::TextDisabled("Set seed BEFORE spawning Brownian particles.");
    }

    ImGui::TextDisabled("Per-particle Brownian tuning is in Sim - Particles.");
}

// ── draw_panel_pursuit ────────────────────────────────────────────────────────

void SurfaceSimScene::draw_panel_pursuit() {
    ImGui::SeparatorText("Delay pursuit  [Ctrl+R]");
    {
        auto& p = m_dp_params;
        ImGui::SliderFloat("Tau (delay)##dp", &p.tau,           0.1f, 10.f, "%.2f s");
        ImGui::SliderFloat("Speed##dp",        &p.pursuit_speed, 0.1f,  3.f, "%.2f");
        ImGui::SliderFloat("Noise sigma##dp",  &p.noise_sigma,   0.f,   1.f, "%.3f");
        if (ImGui::Button("Spawn pursuer  [Ctrl+R]", ImVec2(-1.f, 0.f))) {
            if (!m_curves.empty()) {
                if (m_curves[0].history() == nullptr) {
                    const std::size_t cap =
                        static_cast<std::size_t>(std::ceil(p.tau * 120.f * 1.5f)) + 256;
                    m_curves[0].enable_history(cap, 1.f / 120.f);
                }
                const Vec3 ref = m_curves[0].head_world();
                const f32 ang  = static_cast<f32>(m_dp_count) * 1.1f + 0.3f;
                const f32 sx   = std::clamp(ref.x + 2.f*ops::cos(ang),
                    m_surface->u_min()+0.5f, m_surface->u_max()-0.5f);
                const f32 sy   = std::clamp(ref.y + 2.f*ops::sin(ang),
                    m_surface->v_min()+0.5f, m_surface->v_max()-0.5f);
                auto builder = m_particles.factory().particle();
                builder
                    .named("Chaser - Delay Pursuit")
                    .role(ParticleRole::Chaser)
                    .at({sx, sy})
                    .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
                    .stochastic()
                    .with_equation(std::make_unique<ndde::sim::DelayPursuitEquation>(
                        m_curves[0].history(), m_surface.get(), p));
                m_particles.spawn(std::move(builder));
                ++m_chaser_count;  ++m_dp_count;
            }
        }
        if (m_curves.empty() || m_curves[0].history() == nullptr)
            ImGui::TextDisabled("(leader has no history yet -- spawn first)");
        else {
            const auto* h = m_curves[0].history();
            const float window = h->newest_t() - h->oldest_t();
            ImGui::TextDisabled("History: %.1fs  %zu records", window, h->size());
            if (window < p.tau)
                ImGui::TextColored({1.f,0.8f,0.1f,1.f},
                    "Warming up: %.1f/%.1fs", window, p.tau);
        }
    }

    ImGui::SeparatorText("Collision avoidance");
    {
        bool changed = ImGui::Checkbox("Min-distance push##col", &m_pair_collision);
        if (m_pair_collision)
            changed |= ImGui::SliderFloat("Min dist##col", &m_min_dist, 0.05f, 2.f, "%.2f");
        if (changed) {
            m_pair_constraints.clear();
            if (m_pair_collision)
                m_pair_constraints.push_back(
                    std::make_unique<ndde::sim::MinDistConstraint>(m_min_dist));
        }
    }

    // Leader seeker section: only visible on Extremum surface
    draw_leader_seeker_panel();

}

// ── draw_panel_geometry ───────────────────────────────────────────────────────

void SurfaceSimScene::draw_panel_geometry() {
    if (m_curves.empty() || !m_curves[0].has_trail()) return;
    const AnimatedCurve& c0 = m_curves[0];
    const u32 hi = c0.trail_size() > 2 ? c0.trail_size()-2 : 0;
    const FrenetFrame fr = c0.frenet_at(hi);
    const Vec3 hp = c0.head_world();

    ImGui::SeparatorText("At head (particle 0)");
    if (m_surface_type == SurfaceType::Gaussian)
        ImGui::TextDisabled("f(x,y) = %.4f", m_surface->evaluate(hp.x, hp.y, m_sim_time).z);
    else
        ImGui::TextDisabled("p = (%.3f, %.3f, %.3f)",
            m_surface->evaluate(hp.x, hp.y, m_sim_time).x,
            m_surface->evaluate(hp.x, hp.y, m_sim_time).y,
            m_surface->evaluate(hp.x, hp.y, m_sim_time).z);

    ImGui::SeparatorText("Frenet\xe2\x80\x93Serret");
    ImGui::TextColored(ImVec4(1.f,0.5f,0.1f,1.f), "T (%.3f, %.3f, %.3f)", fr.T.x, fr.T.y, fr.T.z);
    ImGui::TextColored(ImVec4(0.2f,1.f,0.4f,1.f), "N (%.3f, %.3f, %.3f)", fr.N.x, fr.N.y, fr.N.z);
    ImGui::TextColored(ImVec4(0.3f,0.6f,1.f,1.f), "B (%.3f, %.3f, %.3f)", fr.B.x, fr.B.y, fr.B.z);
    ImGui::TextDisabled("\xce\xba = %.5f  osc.r = %.4f",
        fr.kappa, fr.kappa > 1e-5f ? 1.f/fr.kappa : 0.f);
    ImGui::TextDisabled("\xcf\x84 = %.5f", fr.tau);

    ImGui::SeparatorText("Curvature");
    const f32 K = m_surface->gaussian_curvature(hp.x, hp.y, m_sim_time);
    const f32 H = m_surface->mean_curvature(hp.x, hp.y, m_sim_time);
    ImGui::TextDisabled("K = %.5f  H = %.5f", K, H);

    ImGui::SeparatorText("First fundamental form");
    const SurfaceFrame sf = make_surface_frame(*m_surface, hp.x, hp.y, m_sim_time, &fr);
    ImGui::TextDisabled("E=%.4f  F=%.4f  G=%.4f", sf.E, sf.F, sf.G);
    ImGui::TextDisabled("\xce\xba_n=%.5f  \xce\xba_g=%.5f", sf.kappa_n, sf.kappa_g);
    const f32 k2    = fr.kappa*fr.kappa;
    const f32 check = sf.kappa_n*sf.kappa_n + sf.kappa_g*sf.kappa_g;
    const bool ok   = std::abs(k2-check) < 1e-4f || k2 < 1e-8f;
    ImGui::TextColored(ok ? ImVec4(.4f,1.f,.4f,1.f) : ImVec4(1.f,.3f,.3f,1.f),
        "\xce\xba\xc2\xb2=\xce\xba_n\xc2\xb2+\xce\xba_g\xc2\xb2 %s", ok ? "\xe2\x9c\x93" : "!");
}

// ── draw_panel_camera ─────────────────────────────────────────────────────────

void SurfaceSimScene::draw_panel_camera() {
    ImGui::SliderFloat("Yaw",      &m_vp3d.yaw,
                       -std::numbers::pi_v<f32>, std::numbers::pi_v<f32>, "%.2f");
    ImGui::SliderFloat("Pitch",    &m_vp3d.pitch,  -1.5f, 1.5f, "%.2f");
    ImGui::SliderFloat("Zoom##3d", &m_vp3d.zoom,   0.1f,  5.f,  "%.2f");
    if (ImGui::Button("Reset 3D")) m_vp3d.reset();
    ImGui::SameLine();
    if (ImGui::Button("Reset 2D")) m_vp2d.reset();
}

// ── draw_surface_3d_window ────────────────────────────────────────────────────

void SurfaceSimScene::draw_surface_3d_window() {
    ImGui::SetNextWindowPos(ImVec2(330.f, 20.f),  ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(750.f, 660.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoBackground;
    ImGui::Begin("Surface 3D", nullptr, flags);

    const ImVec2 cpos = ImGui::GetCursorScreenPos();
    const ImVec2 csz  = ImGui::GetContentRegionAvail();

    m_vp3d.fb_w = csz.x;  m_vp3d.fb_h = csz.y;
    m_vp3d.dp_w = csz.x;  m_vp3d.dp_h = csz.y;
    m_canvas3d_pos = cpos;
    m_canvas3d_sz  = csz;

    ImGui::InvisibleButton("3d_canvas", csz,
        ImGuiButtonFlags_MouseButtonLeft  |
        ImGuiButtonFlags_MouseButtonRight |
        ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();

    if (hovered) {
        const ImGuiIO& io = ImGui::GetIO();
        if (std::abs(io.MouseWheel) > 0.f)
            m_vp3d.zoom = std::clamp(m_vp3d.zoom*(1.f + 0.12f*io.MouseWheel), 0.05f, 20.f);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
            m_vp3d.orbit(io.MouseDelta.x, io.MouseDelta.y);
        update_hover(cpos, csz, true);
    } else {
        m_snap_on_curve = false;
        m_snap_idx      = -1;
    }

    const Mat4 mvp3d = canvas_mvp_3d(cpos, csz);

    switch (m_surface_display) {
        case SurfaceDisplay::Wireframe: submit_wireframe_3d(mvp3d); break;
        case SurfaceDisplay::Filled:    submit_filled_3d(mvp3d);    break;
        case SurfaceDisplay::Both:
            submit_filled_3d(mvp3d);
            submit_wireframe_3d(mvp3d);
            break;
    }

    // Sync flags and delegate particle rendering to ParticleRenderer (B2).
    m_particle_renderer.show_frenet       = m_show_frenet;
    m_particle_renderer.show_T            = m_show_T;
    m_particle_renderer.show_N            = m_show_N;
    m_particle_renderer.show_B            = m_show_B;
    m_particle_renderer.show_osc          = m_show_osc;
    m_particle_renderer.show_dir_deriv    = m_show_dir_deriv;
    m_particle_renderer.show_normal_plane = m_show_normal_plane;
    m_particle_renderer.show_torsion      = m_show_torsion;
    m_particle_renderer.frame_scale       = m_frame_scale;
    m_particle_renderer.submit_all(m_curves, *m_surface, m_sim_time,
                                   mvp3d, m_snap_curve, m_snap_idx, m_snap_on_curve);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddText(ImVec2(cpos.x+8, cpos.y+6),
        IM_COL32(200,200,200,180),
        "Right-drag: orbit   Scroll: zoom   Ctrl+L: add particle");

    if (m_sim_paused)
        dl->AddText(ImVec2(cpos.x+8, cpos.y+24),
            IM_COL32(255, 210, 60, 240), "PAUSED  [Ctrl+P to resume]");

    if (m_snap_on_curve && m_snap_idx >= 0 && m_snap_curve >= 0 &&
        m_snap_curve < static_cast<int>(m_curves.size()))
    {
        const FrenetFrame fr = m_curves[m_snap_curve].frenet_at(
            static_cast<u32>(m_snap_idx));
        char buf[128];
        std::snprintf(buf, sizeof(buf), "k=%.4f  t=%.4f  osc.r=%.3f",
            fr.kappa, fr.tau, fr.kappa>1e-5f ? 1.f/fr.kappa : 0.f);
        dl->AddText(ImVec2(cpos.x+8, cpos.y+22), IM_COL32(255,220,100,220), buf);
    }

    ImGui::End();
}

// ── draw_contour_2d_window ────────────────────────────────────────────────────

void SurfaceSimScene::draw_contour_2d_window() {
    // Not called from on_frame() -- second window handles 2D rendering.
}

// ── update_hover ──────────────────────────────────────────────────────────────

void SurfaceSimScene::update_hover(const ImVec2& canvas_pos,
                                    const ImVec2& canvas_size,
                                    bool is_3d)
{
    const ImGuiIO& io    = ImGui::GetIO();
    const f32      mx    = io.MousePos.x - canvas_pos.x;
    const f32      my    = io.MousePos.y - canvas_pos.y;
    constexpr f32  SNAP  = 20.f;

    m_snap_idx      = -1;
    m_snap_curve    = 0;
    m_snap_on_curve = false;

    if (m_curves.empty()) return;

    if (is_3d) {
        const Mat4 mvp   = canvas_mvp_3d(canvas_pos, canvas_size);
        const Vec2 sw_sz = m_api.viewport_size();
        const f32  sw    = sw_sz.x > 0.f ? sw_sz.x : 1.f;
        const f32  sh    = sw_sz.y > 0.f ? sw_sz.y : 1.f;
        f32 best = SNAP;
        for (u32 ci = 0; ci < static_cast<u32>(m_curves.size()); ++ci) {
            const AnimatedCurve& c = m_curves[ci];
            for (u32 i = 0; i < c.trail_size(); ++i) {
                const Vec3 wp = c.trail_pt(i);
                const glm::vec4 clip = mvp * glm::vec4(wp.x, wp.y, wp.z, 1.f);
                if (clip.w <= 0.f) continue;
                const f32 px = (clip.x/clip.w + 1.f) * 0.5f * sw - canvas_pos.x;
                const f32 py = (1.f - clip.y/clip.w) * 0.5f * sh - canvas_pos.y;
                const f32 d  = std::hypot(px - mx, py - my);
                if (d < best) { best = d; m_snap_idx = (int)i; m_snap_curve = (int)ci; }
            }
        }
    } else {
        f32 best = SNAP;
        for (u32 ci = 0; ci < static_cast<u32>(m_curves.size()); ++ci) {
            const AnimatedCurve& c = m_curves[ci];
            for (u32 i = 0; i < c.trail_size(); ++i) {
                const Vec3 wp = c.trail_pt(i);
                const Vec2 sp = m_vp2d.world_to_pixel(wp.x, wp.y);
                const f32  d  = std::hypot(sp.x-mx, sp.y-my);
                if (d < best) { best = d; m_snap_idx = (int)i; m_snap_curve = (int)ci; }
            }
        }
    }

    if (m_snap_idx >= 0) m_snap_on_curve = true;
}

// ── Surface geometry caches ───────────────────────────────────────────────────

namespace {

SurfaceMeshOptions surface_mesh_options(u32 grid_lines, float time, float color_scale) {
    return SurfaceMeshOptions{
        .grid_lines = grid_lines,
        .time = time,
        .color_scale = color_scale,
        .wire_color = {1.f, 1.f, 1.f, 1.f},
        .fill_color_mode = SurfaceFillColorMode::GaussianCurvatureCell,
        .build_contour = false
    };
}

} // namespace

void SurfaceSimScene::submit_wireframe_3d(const Mat4& mvp) {
    m_mesh.rebuild_if_needed(*m_surface, surface_mesh_options(m_grid_lines, m_sim_time, m_curv_scale));
    if (m_mesh.wire_count() == 0) return;
    auto slice = m_api.acquire(m_mesh.wire_count());
    std::memcpy(slice.vertices(), m_mesh.wire_vertices().data(),
                m_mesh.wire_count() * sizeof(Vertex));
    m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp);
}

void SurfaceSimScene::submit_filled_3d(const Mat4& mvp) {
    m_mesh.rebuild_if_needed(*m_surface, surface_mesh_options(m_grid_lines, m_sim_time, m_curv_scale));
    if (m_mesh.fill_count() == 0) return;
    auto slice = m_api.acquire(m_mesh.fill_count());
    std::memcpy(slice.vertices(), m_mesh.fill_vertices().data(),
                m_mesh.fill_count() * sizeof(Vertex));
    m_api.submit_to(RenderTarget::Primary3D, slice, Topology::TriangleList, DrawMode::VertexColor, {1,1,1,1}, mvp);
}

// ── submit_contour_second_window ──────────────────────────────────────────────

void SurfaceSimScene::submit_contour_second_window() {
    const Vec2 sz2 = m_api.viewport_size2();
    if (sz2.x <= 0.f || sz2.y <= 0.f) return;

    const f32 domain = m_surface->u_max();
    const f32 aspect = sz2.x / sz2.y;
    const f32 half_y = domain / m_vp2d.zoom;
    const f32 half_x = half_y * aspect;
    const Mat4 mvp2  = glm::ortho(
        m_vp2d.pan_x - half_x, m_vp2d.pan_x + half_x,
        m_vp2d.pan_y - half_y, m_vp2d.pan_y + half_y,
        -10.f, 10.f);

    const auto* gs = dynamic_cast<const GaussianSurface*>(m_surface.get());
    if (gs) {
        constexpr u32 NX=80, NY=80;
        auto slice = m_api.acquire(NX*NY*6);
        Vertex* v  = slice.vertices(); u32 idx = 0;
        const f32 dx = (gs->u_max()-gs->u_min())/NX;
        const f32 dy = (gs->v_max()-gs->v_min())/NY;
        for (u32 i=0;i<NX;++i) for (u32 j=0;j<NY;++j) {
            const f32 x0=gs->u_min()+i*dx, x1=x0+dx;
            const f32 y0=gs->v_min()+j*dy, y1=y0+dy;
            const Vec4 c00=GaussianSurface::height_color(GaussianSurface::eval(x0,y0));
            const Vec4 c10=GaussianSurface::height_color(GaussianSurface::eval(x1,y0));
            const Vec4 c01=GaussianSurface::height_color(GaussianSurface::eval(x0,y1));
            const Vec4 c11=GaussianSurface::height_color(GaussianSurface::eval(x1,y1));
            v[idx++]={Vec3{x0,y0,0},c00}; v[idx++]={Vec3{x1,y0,0},c10}; v[idx++]={Vec3{x1,y1,0},c11};
            v[idx++]={Vec3{x0,y0,0},c00}; v[idx++]={Vec3{x1,y1,0},c11}; v[idx++]={Vec3{x0,y1,0},c01};
        }
        memory::ArenaSlice tr=slice; tr.vertex_count=idx;
        m_api.submit_to(RenderTarget::Contour2D, tr, Topology::TriangleList, DrawMode::VertexColor, {1,1,1,1}, mvp2);

        if (m_show_contours) {
            const u32 max_v = GaussianSurface::contour_max_vertices(100u, k_n_levels);
            auto cslice = m_api.acquire(max_v);
            const u32 actual = GaussianSurface::tessellate_contours(
                {cslice.vertices(),max_v}, 100u, k_levels, k_n_levels, {1,1,1,0.8f});
            if (actual > 0) {
                memory::ArenaSlice tr2=cslice; tr2.vertex_count=actual;
                m_api.submit_to(RenderTarget::Contour2D, tr2, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp2);
            }
        }
    }

    for (const AnimatedCurve& c : m_curves) {
        const u32 nt = c.trail_size();
        if (nt < 2) continue;
        auto slice = m_api.acquire(nt);
        Vertex* vt = slice.vertices();
        for (u32 i = 0; i < nt; ++i) {
            const Vec3 pt = c.trail_pt(i);
            const f32  t  = static_cast<f32>(i)/static_cast<f32>(nt-1);
            vt[i] = { Vec3{pt.x,pt.y,0},
                      AnimatedCurve::trail_colour(c.role(), c.colour_slot(), t) };
        }
        m_api.submit_to(RenderTarget::Contour2D, slice, Topology::LineStrip, DrawMode::VertexColor, {1,1,1,1}, mvp2);
    }

    {
        const int ci = (m_snap_on_curve && m_snap_curve >= 0 &&
                        m_snap_curve < static_cast<int>(m_curves.size()))
                       ? m_snap_curve : 0;
        if (ci < static_cast<int>(m_curves.size())) {
            const AnimatedCurve& ac = m_curves[ci];
            const u32 nt = ac.trail_size();
            const u32 sidx = (m_snap_on_curve && m_snap_curve == ci &&
                              m_snap_idx >= 0 && static_cast<u32>(m_snap_idx) < nt)
                ? static_cast<u32>(m_snap_idx)
                : (nt > 2 ? nt-2 : 0);
            if (nt >= 3 && sidx > 0) {
                const FrenetFrame fr = ac.frenet_at(sidx);
                const Vec3 o3 = ac.trail_pt(sidx);
                const Vec3 o  = {o3.x, o3.y, 0};
                const f32  scl = m_frame_scale * 0.4f;
                auto arr = [&](Vec3 dir, Vec4 col) {
                    if (glm::length(dir) < 1e-5f) return;
                    auto s = m_api.acquire(2);
                    s.vertices()[0] = {o, col};
                    s.vertices()[1] = {o + glm::normalize(dir)*scl, col};
                    m_api.submit_to(RenderTarget::Contour2D, s, Topology::LineList, DrawMode::VertexColor, col, mvp2);
                };
                if (m_show_T) arr({fr.T.x,fr.T.y,0},{1.f,0.35f,0.05f,1.f});
                if (m_show_N) arr({fr.N.x,fr.N.y,0},{0.15f,1.f,0.3f,1.f});
                if (m_show_osc && fr.kappa > 1e-5f) {
                    const f32 R = 1.f/fr.kappa;
                    constexpr u32 SEG = 64;
                    auto sc2 = m_api.acquire(SEG+1);
                    Vertex* vc = sc2.vertices();
                    const Vec3 ctr = {o3.x+fr.N.x*R, o3.y+fr.N.y*R, 0};
                    for (u32 k=0; k<=SEG; ++k) {
                        const f32 th = (static_cast<f32>(k)/SEG)*2.f*std::numbers::pi_v<f32>;
                        vc[k] = { ctr+Vec3{ops::cos(th)*R,ops::sin(th)*R,0}, {0.7f,0.3f,1.f,0.65f} };
                    }
                    m_api.submit_to(RenderTarget::Contour2D, sc2, Topology::LineStrip, DrawMode::VertexColor,
                                  {0.7f,0.3f,1.f,0.65f}, mvp2);
                }
                if (m_show_torsion && fr.kappa > 1e-5f && std::abs(fr.tau) > 1e-4f) {
                    const Vec4 tau_col = fr.tau > 0.f
                        ? Vec4{1.f, 0.55f, 0.1f, 0.9f}
                        : Vec4{0.3f, 0.8f, 1.f,  0.9f};
                    const Vec3 dBds_xy = { -fr.tau*fr.N.x, -fr.tau*fr.N.y, 0.f };
                    arr(dBds_xy, tau_col);
                    const f32 len = std::clamp(std::abs(fr.tau) * scl * 2.f, 0.02f, 1.5f);
                    const Vec3 tip_dir = glm::length(dBds_xy) > 1e-6f
                        ? glm::normalize(dBds_xy) : Vec3{0,1,0};
                    const Vec3 tip = o + tip_dir * len;
                    auto dot = m_api.acquire(1);
                    dot.vertices()[0] = { tip, tau_col };
                    m_api.submit_to(RenderTarget::Contour2D, dot, Topology::LineStrip, DrawMode::UniformColor, tau_col, mvp2);
                }
            }
        }
    }
}

// ── Extremum helpers ──────────────────────────────────────────────────────────

void SurfaceSimScene::rebuild_extremum_table_if_needed() {
    if (m_surface_type != SurfaceType::Extremum) return;
    if (m_extremum_rebuild_countdown == 0) {
        m_extremum_table.build(*m_surface, m_sim_time);
        m_extremum_rebuild_countdown = 30u;
    } else {
        --m_extremum_rebuild_countdown;
    }
}

void SurfaceSimScene::spawn_leader_seeker() {
    const float u_mid = 0.5f*(m_surface->u_min() + m_surface->u_max());
    const float v_mid = 0.5f*(m_surface->v_min() + m_surface->v_max());

    std::unique_ptr<ndde::sim::IEquation> eq;
    if (m_leader_mode == LeaderMode::Deterministic)
        eq = std::make_unique<ndde::sim::LeaderSeekerEquation>(
                &m_extremum_table, m_ls_params);
    else
        eq = std::make_unique<ndde::sim::BiasedBrownianLeader>(
                &m_extremum_table, m_bbl_params);

    // Enable history so pursuers can use it
    const std::size_t cap =
        static_cast<std::size_t>(std::ceil(m_pursuit_tau * 120.f * 1.5f)) + 256;

    auto builder = m_particles.factory().particle();
    builder
        .named(m_leader_mode == LeaderMode::Deterministic
            ? "Leader - Extremum Seeker"
            : "Leader - Biased Brownian")
        .role(ParticleRole::Leader)
        .at({u_mid, v_mid})
        .history(cap, 1.f / 120.f)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_equation(std::move(eq));
    m_particles.spawn(std::move(builder));
    ++m_leader_count;
    m_spawning_pursuer = true;
}

void SurfaceSimScene::spawn_pursuit_particle() {
    if (m_curves.empty()) return;

    // Ensure leader has history
    if (m_curves[0].history() == nullptr) {
        const std::size_t cap =
            static_cast<std::size_t>(std::ceil(m_pursuit_tau * 120.f * 1.5f)) + 256;
        m_curves[0].enable_history(cap, 1.f / 120.f);
    }

    const glm::vec2 ref = m_curves[0].head_uv();
    const glm::vec2 uv  = offset_spawn(ref, 2.0f,
        static_cast<float>(m_dp_count) * 1.1f + 0.3f);

    std::unique_ptr<ndde::sim::IEquation> eq;
    switch (m_pursuit_mode) {
        case PursuitMode::Direct:
            eq = std::make_unique<ndde::sim::DirectPursuitEquation>(
                [this]{ return m_curves[0].head_uv(); },
                ndde::sim::DirectPursuitEquation::Params{
                    m_ls_params.pursuit_speed, 0.f });
            break;
        case PursuitMode::Delayed:
            eq = std::make_unique<ndde::sim::DelayPursuitEquation>(
                m_curves[0].history(), m_surface.get(),
                ndde::sim::DelayPursuitEquation::Params{
                    m_pursuit_tau, m_ls_params.pursuit_speed, 0.f });
            break;
        case PursuitMode::Momentum:
            eq = std::make_unique<ndde::sim::MomentumBearingEquation>(
                m_curves[0].history(),
                ndde::sim::MomentumBearingEquation::Params{
                    m_ls_params.pursuit_speed, m_pursuit_window, 0.f });
            break;
    }

    auto builder = m_particles.factory().particle();
    builder
        .named("Chaser - Pursuit")
        .role(ParticleRole::Chaser)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_equation(std::move(eq));
    m_particles.spawn(std::move(builder));
    ++m_chaser_count;
    ++m_dp_count;
}

void SurfaceSimScene::draw_leader_seeker_panel() {
    if (m_surface_type != SurfaceType::Extremum) return;
    ImGui::SeparatorText("Leader Seeker  [Ctrl+A]");

    // Leader mode
    {
        int mode = static_cast<int>(m_leader_mode);
        bool changed = false;
        changed |= ImGui::RadioButton("Deterministic##lm", &mode, 0);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Biased Brownian##lm", &mode, 1);
        if (changed) m_leader_mode = static_cast<LeaderMode>(mode);
    }

    if (m_leader_mode == LeaderMode::Deterministic) {
        auto& p = m_ls_params;
        ImGui::SliderFloat("Target grad mag##ls",  &p.target_grad_magnitude, 0.f, 2.f,  "%.2f");
        ImGui::SliderFloat("Epsilon##ls",           &p.epsilon,               0.01f, 0.5f, "%.3f");
        ImGui::SliderFloat("Leader speed##ls",      &p.pursuit_speed,         0.1f, 3.f,  "%.2f");
        ImGui::SliderFloat("Leader noise##ls",      &p.noise_sigma,           0.f,  1.f,  "%.3f");
        ImGui::SliderFloat("Arrival radius##ls",    &p.arrival_radius,        0.1f, 2.f,  "%.2f");
    } else {
        auto& p = m_bbl_params;
        ImGui::SliderFloat("Sigma##bbl",             &p.sigma,           0.01f, 2.f,  "%.3f");
        ImGui::SliderFloat("Goal drift##bbl",        &p.drift_strength,  0.f,   2.f,  "%.2f");
        ImGui::SliderFloat("Gradient drift##bbl",    &p.gradient_drift,  -1.f,  1.f,  "%.3f");
        ImGui::SliderFloat("Epsilon##bbl",           &p.epsilon,         0.01f, 0.5f, "%.3f");
        ImGui::SliderFloat("Arrival radius##bbl",    &p.arrival_radius,  0.1f,  2.f,  "%.2f");
        if (p.sigma > 1e-5f)
            ImGui::TextDisabled("Peclet = %.2f  (drift/sigma^2)",
                p.drift_strength / (p.sigma * p.sigma));
    }

    if (ImGui::SmallButton(!m_spawning_pursuer ? "Spawn leader [Ctrl+A]" : "Spawn pursuer [Ctrl+A]")) {
        if (!m_spawning_pursuer) spawn_leader_seeker();
        else                     spawn_pursuit_particle();
    }

    // Pursuit mode
    ImGui::SeparatorText("Pursuit mode");
    {
        int mode = static_cast<int>(m_pursuit_mode);
        bool changed = false;
        changed |= ImGui::RadioButton("Direct##pm",   &mode, 0);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Delayed##pm",  &mode, 1);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Momentum##pm", &mode, 2);
        if (changed) m_pursuit_mode = static_cast<PursuitMode>(mode);
    }
    if (m_pursuit_mode == PursuitMode::Delayed)
        ImGui::SliderFloat("Tau##pm",    &m_pursuit_tau,    0.1f, 10.f, "%.2f s");
    if (m_pursuit_mode == PursuitMode::Momentum)
        ImGui::SliderFloat("Window##pm", &m_pursuit_window, 0.1f,  5.f, "%.2f s");

    // Extremum table readout
    ImGui::SeparatorText("Extremum table");
    if (m_extremum_table.valid) {
        ImGui::TextDisabled("max  u=%.3f v=%.3f  z=%.3f",
            m_extremum_table.max_uv.x, m_extremum_table.max_uv.y, m_extremum_table.max_z);
        ImGui::TextDisabled("min  u=%.3f v=%.3f  z=%.3f",
            m_extremum_table.min_uv.x, m_extremum_table.min_uv.y, m_extremum_table.min_z);
        if (ImGui::SmallButton("Rebuild now##ext"))
            m_extremum_table.build(*m_surface, m_sim_time);
    } else {
        ImGui::TextColored({1.f,0.6f,0.1f,1.f}, "Table invalid -- switch to Extremum surface");
    }
}

// ── export_session ────────────────────────────────────────────────────────────

void SurfaceSimScene::export_session(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << std::fixed << std::setprecision(6);
    f << "particle_id,role,equation,record_type,step,a,b,c\n";

    for (u32 ci = 0; ci < static_cast<u32>(m_curves.size()); ++ci) {
        const auto& c = m_curves[ci];
        const std::string role = (c.role() == AnimatedCurve::Role::Leader) ? "leader" : "chaser";
        const std::string eq   = c.equation() ? c.equation()->name() : "unknown";

        for (u32 i = 0; i < c.trail_size(); ++i) {
            const Vec3 p = c.trail_pt(i);
            f << ci << ',' << role << ',' << eq << ",trail,"
              << i << ',' << p.x << ',' << p.y << ',' << p.z << '\n';
        }
        if (c.history()) {
            const auto recs = c.history()->to_vector();
            for (std::size_t i = 0; i < recs.size(); ++i) {
                f << ci << ',' << role << ',' << eq << ",history,"
                  << i << ',' << recs[i].t << ','
                  << recs[i].uv.x << ',' << recs[i].uv.y << '\n';
            }
        }
    }
}

} // namespace ndde
