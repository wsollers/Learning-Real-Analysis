#include "app/SurfaceSimPanels.hpp"
#include "app/GaussianRipple.hpp"
#include "app/ParticleInspectorPanel.hpp"
#include "app/FrenetFrame.hpp"
#include "numeric/ops.hpp"
#include "sim/MilsteinIntegrator.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace ndde {

SurfaceSimPanels::SurfaceSimPanels(std::unique_ptr<ndde::math::ISurface>& surface,
                                   ParticleSystem& particles,
                                   std::vector<AnimatedCurve>& curves,
                                   Viewport& vp3d,
                                   Viewport& vp2d,
                                   HotkeyManager& hotkeys,
                                   SurfaceSelectionState& surface_state,
                                   SurfaceDisplayState& display,
                                   SurfaceOverlayState& overlays,
                                   SurfaceUiState& ui,
                                   SurfaceSpawnState& spawn,
                                   SurfaceBehaviorParams& behavior,
                                   SurfacePursuitState& pursuit,
                                   SurfaceSimSpawner& spawner,
                                   SurfaceSimController& controller,
                                   bool& paused,
                                   float& sim_time,
                                   float& sim_speed,
                                   GoalStatus& goal_status,
                                   ExportFn export_session)
    : m_surface(surface)
    , m_particles(particles)
    , m_curves(curves)
    , m_vp3d(vp3d)
    , m_vp2d(vp2d)
    , m_hotkeys(hotkeys)
    , m_surface_state(surface_state)
    , m_display(display)
    , m_overlays(overlays)
    , m_ui(ui)
    , m_spawn(spawn)
    , m_behavior(behavior)
    , m_pursuit(pursuit)
    , m_spawner(spawner)
    , m_controller(controller)
    , m_paused(paused)
    , m_sim_time(sim_time)
    , m_sim_speed(sim_speed)
    , m_goal_status(goal_status)
    , m_export_session(std::move(export_session))
{
    register_panels();
}

void SurfaceSimPanels::draw_all() {
    m_host.draw_all();
}

// ── draw_hotkey_panel ─────────────────────────────────────────────────────────

void SurfaceSimPanels::register_panels() {
    m_host.clear();
    m_host.add({
        .title = "Sim - Surface",
        .default_pos = {20.f, 20.f},
        .default_size = {300.f, 220.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_surface(); }
    });
    m_host.add({
        .title = "Sim - Particles",
        .default_pos = {20.f, 250.f},
        .default_size = {300.f, 180.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_particles(); }
    });
    m_host.add({
        .title = "Sim - Overlays",
        .default_pos = {20.f, 440.f},
        .default_size = {300.f, 240.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_overlays(); }
    });
    m_host.add({
        .title = "Sim - Brownian  [Ctrl+B]",
        .default_pos = {20.f, 690.f},
        .default_size = {300.f, 260.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_brownian(); }
    });
    m_host.add({
        .title = "Sim - Pursuit",
        .default_pos = {330.f, 700.f},
        .default_size = {310.f, 320.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_pursuit(); }
    });
    m_host.add({
        .title = "Sim - Geometry",
        .default_pos = {650.f, 700.f},
        .default_size = {310.f, 280.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_geometry(); }
    });
    m_host.add({
        .title = "Sim - Camera",
        .default_pos = {330.f, 20.f},
        .default_size = {310.f, 170.f},
        .bg_alpha = 0.88f,
        .draw_body = [this] { draw_panel_camera(); }
    });
}

void SurfaceSimPanels::draw_hotkey_panel() {
    // Delegate entirely to HotkeyManager. The panel lists every registered
    // binding in group order; no hardcoded row table needed here.
    m_hotkeys.draw_panel("Hotkeys  [Ctrl+H]", m_ui.hotkey_panel_open);
}

// ── draw_panel_surface ────────────────────────────────────────────────────────

void SurfaceSimPanels::draw_panel_surface() {
    ImGui::SeparatorText("Surface type");
    {
        int sel = static_cast<int>(m_surface_state.type);
        bool changed = false;
        changed |= ImGui::RadioButton("Gaussian",       &sel, 0);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Torus",          &sel, 1);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Ripple",         &sel, 2);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Extremum##surf", &sel, 3);
        if (changed) m_controller.swap_surface(static_cast<SurfaceType>(sel));

        if (m_surface_state.type == SurfaceType::Torus) {
            bool tp = false;
            tp |= ImGui::SliderFloat("R##torus", &m_surface_state.torus_R, 0.5f, 5.f, "%.2f");
            tp |= ImGui::SliderFloat("r##torus", &m_surface_state.torus_r, 0.1f, m_surface_state.torus_R - 0.05f, "%.2f");
            if (tp) m_controller.swap_surface(SurfaceType::Torus);
        }

        if (m_surface_state.type == SurfaceType::GaussianRipple) {
            auto& p  = m_surface_state.ripple_params;
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
        int mode = static_cast<int>(m_display.mode);
        bool changed = false;
        changed |= ImGui::RadioButton("Wireframe", &mode, 0);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Filled",    &mode, 1);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Both",      &mode, 2);
        if (changed) {
            m_display.mode = static_cast<SurfaceDisplay>(mode);
            m_display.mesh.mark_dirty();
        }
        if (m_display.mode != SurfaceDisplay::Wireframe) {
            if (ImGui::SliderFloat("K scale##curv", &m_display.curv_scale, 0.01f, 20.f, "%.2f"))
                m_display.mesh.mark_dirty();
        }
        {
            int gl = static_cast<int>(m_display.grid_lines);
            if (ImGui::SliderInt("Grid lines##surface", &gl, 8, 256)) {
                m_display.grid_lines = static_cast<u32>(std::max(gl, 8));
                m_display.mesh.mark_dirty();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("~%uk verts", (4u * m_display.grid_lines * (m_display.grid_lines + 1u)) / 1000u);
        }
    }
}

// ── draw_panel_particles ──────────────────────────────────────────────────────

void SurfaceSimPanels::draw_panel_particles() {
    ImGui::Checkbox("Paused  [Ctrl+P]", &m_paused);
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        m_sim_time = 0.f;
        m_spawner.spawn_showcase_service();
    }
    ImGui::SliderFloat("Speed##sim",  &m_sim_speed,   0.1f, 5.f,  "%.2f");
    ImGui::SliderFloat("Arrow scale", &m_display.frame_scale, 0.05f,0.8f, "%.2f");
    ImGui::Checkbox("Contour lines",  &m_display.show_contours);

    ImGui::Separator();
    ImGui::TextDisabled("%zu particles  (L=%u  C=%u)",
        m_curves.size(), m_spawn.leader_count, m_spawn.chaser_count);
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear all")) {
        m_sim_time = 0.f;
        m_spawner.spawn_showcase_service();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Export CSV")) {
        const std::string path = "session_" +
            std::to_string(m_curves.size()) + "p_" +
            std::to_string(static_cast<int>(m_sim_time)) + "s.csv";
        m_export_session(path);
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

void SurfaceSimPanels::draw_panel_overlays() {
    ImGui::SeparatorText("Frenet frame  [Ctrl+F]");
    ImGui::Checkbox("Show Frenet", &m_overlays.show_frenet);
    ImGui::BeginDisabled(!m_overlays.show_frenet);
    ImGui::SameLine(); ImGui::Checkbox("T##ft", &m_overlays.show_T);
    ImGui::SameLine(); ImGui::Checkbox("N##fn", &m_overlays.show_N);
    ImGui::SameLine(); ImGui::Checkbox("B##fb", &m_overlays.show_B);
    ImGui::Checkbox("Osculating circle", &m_overlays.show_osc);
    ImGui::EndDisabled();

    ImGui::SeparatorText("Other overlays");
    ImGui::Checkbox("Surface frame  [Ctrl+D]", &m_overlays.show_dir_deriv);
    ImGui::Checkbox("Normal plane   [Ctrl+N]", &m_overlays.show_normal_plane);
    ImGui::Checkbox("Torsion ribbon [Ctrl+T]", &m_overlays.show_torsion);

    // Torsion live readout when active
    if (m_overlays.show_torsion && !m_curves.empty() && m_curves[0].has_trail()) {
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

void SurfaceSimPanels::draw_panel_brownian() {
    auto& p = m_behavior.brownian;
    ImGui::SliderFloat("Sigma##bm", &p.sigma,          0.01f, 2.f, "%.3f");
    ImGui::SliderFloat("Drift##bm", &p.drift_strength, -1.f,  1.f, "%.3f");
    if (ImGui::Button("Spawn Brownian  [Ctrl+B]", ImVec2(-1.f, 0.f))) {
        m_spawner.spawn_brownian_particle(m_spawner.offset_spawn(m_spawner.reference_uv(), 1.8f,
            static_cast<float>(m_spawn.chaser_count + m_spawn.leader_count) * 0.7f + 1.0f));
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

void SurfaceSimPanels::draw_panel_pursuit() {
    ImGui::SeparatorText("Delay pursuit  [Ctrl+R]");
    {
        auto& p = m_behavior.delay_pursuit;
        ImGui::SliderFloat("Tau (delay)##dp", &p.tau,           0.1f, 10.f, "%.2f s");
        ImGui::SliderFloat("Speed##dp",        &p.pursuit_speed, 0.1f,  3.f, "%.2f");
        ImGui::SliderFloat("Noise sigma##dp",  &p.noise_sigma,   0.f,   1.f, "%.3f");
        if (ImGui::Button("Spawn pursuer  [Ctrl+R]", ImVec2(-1.f, 0.f))) {
            if (!m_curves.empty()) {
                const glm::vec2 ref = m_curves[0].head_uv();
                m_spawner.spawn_delay_pursuit_particle(m_spawner.offset_spawn(ref, 2.0f,
                    static_cast<float>(m_spawn.delay_pursuit_count) * 1.1f + 0.3f));
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
        bool changed = ImGui::Checkbox("Min-distance push##col", &m_pursuit.pair_collision);
        if (m_pursuit.pair_collision)
            changed |= ImGui::SliderFloat("Min dist##col", &m_pursuit.min_dist, 0.05f, 2.f, "%.2f");
        if (changed)
            m_controller.sync_pairwise_constraints();
    }

    // Leader seeker section: only visible on Extremum surface
    draw_leader_seeker_panel();

}

void SurfaceSimPanels::draw_leader_seeker_panel() {
    if (m_surface_state.type != SurfaceType::Extremum) return;
    ImGui::SeparatorText("Leader Seeker  [Ctrl+A]");

    {
        int mode = static_cast<int>(m_pursuit.leader_mode);
        bool changed = false;
        changed |= ImGui::RadioButton("Deterministic##lm", &mode, 0);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Biased Brownian##lm", &mode, 1);
        if (changed) m_pursuit.leader_mode = static_cast<LeaderMode>(mode);
    }

    if (m_pursuit.leader_mode == LeaderMode::Deterministic) {
        auto& p = m_pursuit.leader_seeker;
        ImGui::SliderFloat("Target grad mag##ls",  &p.target_grad_magnitude, 0.f, 2.f,  "%.2f");
        ImGui::SliderFloat("Epsilon##ls",           &p.epsilon,               0.01f, 0.5f, "%.3f");
        ImGui::SliderFloat("Leader speed##ls",      &p.pursuit_speed,         0.1f, 3.f,  "%.2f");
        ImGui::SliderFloat("Leader noise##ls",      &p.noise_sigma,           0.f,  1.f,  "%.3f");
        ImGui::SliderFloat("Arrival radius##ls",    &p.arrival_radius,        0.1f, 2.f,  "%.2f");
    } else {
        auto& p = m_pursuit.biased_brownian_leader;
        ImGui::SliderFloat("Sigma##bbl",             &p.sigma,           0.01f, 2.f,  "%.3f");
        ImGui::SliderFloat("Goal drift##bbl",        &p.drift_strength,  0.f,   2.f,  "%.2f");
        ImGui::SliderFloat("Gradient drift##bbl",    &p.gradient_drift,  -1.f,  1.f,  "%.3f");
        ImGui::SliderFloat("Epsilon##bbl",           &p.epsilon,         0.01f, 0.5f, "%.3f");
        ImGui::SliderFloat("Arrival radius##bbl",    &p.arrival_radius,  0.1f,  2.f,  "%.2f");
        if (p.sigma > 1e-5f)
            ImGui::TextDisabled("Peclet = %.2f  (drift/sigma^2)",
                p.drift_strength / (p.sigma * p.sigma));
    }

    if (ImGui::SmallButton(!m_spawn.spawning_pursuer ? "Spawn leader [Ctrl+A]" : "Spawn pursuer [Ctrl+A]")) {
        if (!m_spawn.spawning_pursuer) m_spawner.spawn_leader_seeker();
        else                          m_spawner.spawn_pursuit_particle();
    }

    ImGui::SeparatorText("Pursuit mode");
    {
        int mode = static_cast<int>(m_pursuit.pursuit_mode);
        bool changed = false;
        changed |= ImGui::RadioButton("Direct##pm",   &mode, 0);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Delayed##pm",  &mode, 1);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Momentum##pm", &mode, 2);
        if (changed) m_pursuit.pursuit_mode = static_cast<PursuitMode>(mode);
    }
    if (m_pursuit.pursuit_mode == PursuitMode::Delayed)
        ImGui::SliderFloat("Tau##pm",    &m_pursuit.pursuit_tau,    0.1f, 10.f, "%.2f s");
    if (m_pursuit.pursuit_mode == PursuitMode::Momentum)
        ImGui::SliderFloat("Window##pm", &m_pursuit.pursuit_window, 0.1f,  5.f, "%.2f s");

    ImGui::SeparatorText("Extremum table");
    if (m_pursuit.extremum_table.valid) {
        ImGui::TextDisabled("max  u=%.3f v=%.3f  z=%.3f",
            m_pursuit.extremum_table.max_uv.x, m_pursuit.extremum_table.max_uv.y, m_pursuit.extremum_table.max_z);
        ImGui::TextDisabled("min  u=%.3f v=%.3f  z=%.3f",
            m_pursuit.extremum_table.min_uv.x, m_pursuit.extremum_table.min_uv.y, m_pursuit.extremum_table.min_z);
        if (ImGui::SmallButton("Rebuild now##ext"))
            m_pursuit.extremum_table.build(*m_surface, m_sim_time);
    } else {
        ImGui::TextColored({1.f,0.6f,0.1f,1.f}, "Table invalid -- switch to Extremum surface");
    }
}

// ── draw_panel_geometry ───────────────────────────────────────────────────────

void SurfaceSimPanels::draw_panel_geometry() {
    if (m_curves.empty() || !m_curves[0].has_trail()) return;
    const AnimatedCurve& c0 = m_curves[0];
    const u32 hi = c0.trail_size() > 2 ? c0.trail_size()-2 : 0;
    const FrenetFrame fr = c0.frenet_at(hi);
    const Vec3 hp = c0.head_world();

    ImGui::SeparatorText("At head (particle 0)");
    if (m_surface_state.type == SurfaceType::Gaussian)
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

void SurfaceSimPanels::draw_panel_camera() {
    ImGui::SliderFloat("Yaw",      &m_vp3d.yaw,
                       -std::numbers::pi_v<f32>, std::numbers::pi_v<f32>, "%.2f");
    ImGui::SliderFloat("Pitch",    &m_vp3d.pitch,  -1.5f, 1.5f, "%.2f");
    ImGui::SliderFloat("Zoom##3d", &m_vp3d.zoom,   0.1f,  5.f,  "%.2f");
    if (ImGui::Button("Reset 3D")) m_vp3d.reset();
    ImGui::SameLine();
    if (ImGui::Button("Reset 2D")) m_vp2d.reset();
}

} // namespace ndde
