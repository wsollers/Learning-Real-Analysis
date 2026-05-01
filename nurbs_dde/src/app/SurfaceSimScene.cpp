// app/SurfaceSimScene.cpp
#include "app/SurfaceSimScene.hpp"
#include "sim/GradientWalker.hpp"   // Step 4: default particle equation
#include "sim/EulerIntegrator.hpp"   // Step 5: Euler integration scheme
#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstring>
#include <numbers>
#include <format>
#include <algorithm>

namespace ndde {

// ── Constructor ───────────────────────────────────────────────────────────────

SurfaceSimScene::SurfaceSimScene(EngineAPI api)
    : m_api(std::move(api))
    , m_surface(std::make_unique<GaussianSurface>())  // Step 3: surface owned here
{
    // 3D camera: dist = base_extent/zoom * 3 = 6/1.0*3 = 18.
    // Surface spans ±6 in XY; 45° FOV, dist 18 → half-angle tangent 6/18 = 0.33 → fits well.
    m_vp3d.base_extent = 6.f;
    m_vp3d.zoom        = 1.0f;
    m_vp3d.yaw         = 0.55f;
    m_vp3d.pitch       = 0.42f;

    // 2D ortho viewport — domain [-6,6]x[-6,6], half_h = base_extent/zoom = 6
    m_vp2d.base_extent = 6.f;
    m_vp2d.zoom        = 1.f;
    m_vp2d.pan_x       = 0.f;
    m_vp2d.pan_y       = 0.f;

    // First particle is a Leader, slot 0 (brightest blue), pre-warmed.
    // Step 2: pass &m_gaussian_surface so the particle uses ISurface polymorphism.
    m_curves.emplace_back(-4.5f, -4.0f, AnimatedCurve::Role::Leader, 0u, m_surface.get(), &m_equation, &m_integrator);
    m_leader_count = 1;
    for (int i = 0; i < 400; ++i)
        m_curves[0].advance(1.f/60.f, 1.f);
}

// ── on_frame ──────────────────────────────────────────────────────────────────

void SurfaceSimScene::on_frame(f32 dt) {
    handle_hotkeys();
    advance_simulation(dt);

    // ── Fullscreen dockspace ──────────────────────────────────────────────────
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

    if (!m_dock_built) {
        m_dock_built = true;
        ImGui::DockBuilderRemoveNode(dock_id);
        ImGui::DockBuilderAddNode(dock_id,
            static_cast<ImGuiDockNodeFlags>(ImGuiDockNodeFlags_DockSpace));
        ImGui::DockBuilderSetNodeSize(dock_id, vp->WorkSize);
        ImGui::DockBuilderDockWindow("Surface 3D", dock_id);
        ImGui::DockBuilderFinish(dock_id);
    }
    ImGui::End();

    draw_simulation_panel();
    draw_surface_3d_window();
    submit_contour_second_window();
    draw_hotkey_panel();

    struct StubConic { std::string name; std::vector<int> snap_cache; };
    const std::vector<StubConic> no_conics;
    m_coord_debug.update(m_vp3d, m_hover, no_conics, m_api.viewport_size());
    m_coord_debug.draw();
    m_debug_open = m_coord_debug.visible();
    m_perf.draw(m_api.debug_stats());
}

// ── swap_surface ─────────────────────────────────────────────────────────────
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
    }
    m_surface_type = type;
    m_sim_time     = 0.f;
    m_wireframe_dirty = true;  // surface geometry changed -- cache must be rebuilt

    // Spread particles across the new domain using a golden-ratio sequence.
    const u32 n    = static_cast<u32>(m_curves.size());
    const f32 u0   = m_surface->u_min(), u1 = m_surface->u_max();
    const f32 v0   = m_surface->v_min(), v1 = m_surface->v_max();
    const f32 um   = m_surface->is_periodic_u() ? 0.f : (u1-u0)*0.08f;
    const f32 vm   = m_surface->is_periodic_v() ? 0.f : (v1-v0)*0.08f;
    const f32 u_lo = u0+um, u_hi = u1-um;
    const f32 v_lo = v0+vm, v_hi = v1-vm;

    for (u32 i = 0; i < n; ++i) {
        const f32 su = (static_cast<f32>(i)+0.5f)/static_cast<f32>(std::max(n,1u));
        const f32 sv = std::fmod(su * 2.618033988f, 1.f);  // golden ratio
        AnimatedCurve fresh(u_lo + su*(u_hi-u_lo), v_lo + sv*(v_hi-v_lo),
                            m_curves[i].role(), m_curves[i].colour_slot(),
                            m_surface.get(), &m_equation, &m_integrator);
        for (int w = 0; w < 120; ++w)
            fresh.advance(1.f/60.f, m_sim_speed);
        m_curves[i] = std::move(fresh);
    }
}

// ── advance_simulation ────────────────────────────────────────────────────────

void SurfaceSimScene::advance_simulation(f32 dt) {
    if (!m_sim_paused) {
        // Tick deformable surfaces BEFORE particle integration.
        if (auto* def = dynamic_cast<ndde::math::IDeformableSurface*>(m_surface.get()))
            def->advance(dt * m_sim_speed);
        m_sim_time += dt * m_sim_speed;

        for (auto& c : m_curves)
            c.advance(dt, m_sim_speed);

        // Step 10: push history for any particle that has recording enabled.
        // This must happen AFTER advance() so the pushed position is current.
        for (auto& c : m_curves)
            c.push_history(m_sim_time);
    }
}

// ── handle_hotkeys ────────────────────────────────────────────────────────────

void SurfaceSimScene::handle_hotkeys() {
    const ImGuiIO& io = ImGui::GetIO();

    auto toggle = [](bool cur, bool& prev, bool& flag) {
        if (cur && !prev) flag = !flag;
        prev = cur;
    };

    toggle(io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_F), m_ctrl_f_prev, m_show_frenet);
    toggle(io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_D), m_ctrl_d_prev, m_show_dir_deriv);
    toggle(io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_N), m_ctrl_n_prev, m_show_normal_plane);  // Ctrl+N: normal plane (was Ctrl+P)
    toggle(io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_T), m_ctrl_t_prev, m_show_torsion);

    // Ctrl+P: pause / unpause the simulation.
    // advance_simulation() is gated on m_sim_paused, so hover, tooltip snap,
    // and all ImGui drawing continue unaffected -- only particle integration stops.
    const bool ctrl_p = io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_P);
    if (ctrl_p && !m_ctrl_p_prev)
        m_sim_paused = !m_sim_paused;
    m_ctrl_p_prev = ctrl_p;

    // Ctrl+L: spawn a new Leader (blue trail).
    const bool ctrl_l = io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_L);
    if (ctrl_l && !m_ctrl_l_prev) {
        const Vec3 ref = m_curves.empty()
            ? Vec3{0.f, 0.f, 0.f}
            : m_curves[0].head_world();
        const f32 off = 1.5f;
        const f32 ang = static_cast<f32>(m_leader_count) * 1.1f;
        const f32 sx  = std::clamp(ref.x + off * std::cos(ang),
                                   m_surface->u_min() + 0.5f,
                                   m_surface->u_max() - 0.5f);
        const f32 sy  = std::clamp(ref.y + off * std::sin(ang),
                                   m_surface->v_min() + 0.5f,
                                   m_surface->v_max() - 0.5f);
        const u32 slot = m_leader_count % AnimatedCurve::MAX_SLOTS;
        m_curves.emplace_back(sx, sy, AnimatedCurve::Role::Leader, slot, m_surface.get(), &m_equation, &m_integrator);
        ++m_leader_count;
        for (int i = 0; i < 60; ++i)
            m_curves.back().advance(1.f/60.f, m_sim_speed);
    }
    m_ctrl_l_prev = ctrl_l;

    // Ctrl+C: spawn a new Chaser (red trail).
    const bool ctrl_c = io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_C);
    if (ctrl_c && !m_ctrl_c_prev) {
        const Vec3 ref = m_curves.empty()
            ? Vec3{0.f, 0.f, 0.f}
            : m_curves[0].head_world();
        const f32 off = 2.0f;
        const f32 ang = static_cast<f32>(m_chaser_count) * 1.3f + 0.5f;
        const f32 sx  = std::clamp(ref.x + off * std::cos(ang),
                                   m_surface->u_min() + 0.5f,
                                   m_surface->u_max() - 0.5f);
        const f32 sy  = std::clamp(ref.y + off * std::sin(ang),
                                   m_surface->v_min() + 0.5f,
                                   m_surface->v_max() - 0.5f);
        const u32 slot = m_chaser_count % AnimatedCurve::MAX_SLOTS;
        m_curves.emplace_back(sx, sy, AnimatedCurve::Role::Chaser, slot, m_surface.get(), &m_equation, &m_integrator);
        ++m_chaser_count;
        for (int i = 0; i < 60; ++i)
            m_curves.back().advance(1.f/60.f, m_sim_speed);
    }
    m_ctrl_c_prev = ctrl_c;

    // Ctrl+B: spawn a Brownian motion particle (owns its equation, uses Milstein).
    const bool ctrl_b = io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_B);
    if (ctrl_b && !m_ctrl_b_prev) {
        const Vec3 ref = m_curves.empty()
            ? Vec3{0.f, 0.f, 0.f}
            : m_curves[0].head_world();
        // Place slightly offset from the reference particle
        const f32 ang = static_cast<f32>(m_chaser_count + m_leader_count) * 0.7f + 1.0f;
        const f32 off = 1.8f;
        const f32 sx  = std::clamp(ref.x + off * std::cos(ang),
                                   m_surface->u_min() + 0.5f,
                                   m_surface->u_max() - 0.5f);
        const f32 sy  = std::clamp(ref.y + off * std::sin(ang),
                                   m_surface->v_min() + 0.5f,
                                   m_surface->v_max() - 0.5f);
        const u32 slot = m_chaser_count % AnimatedCurve::MAX_SLOTS;
        // Spawn with owned BrownianMotion equation using current UI params.
        m_curves.push_back(
            AnimatedCurve::with_equation(
                sx, sy,
                AnimatedCurve::Role::Chaser, slot,
                m_surface.get(),
                std::make_unique<ndde::sim::BrownianMotion>(m_bm_params),
                &m_milstein));
        ++m_chaser_count;
        for (int i = 0; i < 60; ++i)
            m_curves.back().advance(1.f/60.f, m_sim_speed);
    }
    m_ctrl_b_prev = ctrl_b;

    // Ctrl+R: spawn a delay-pursuit chaser targeting curve 0 (the Leader).
    // The chaser pursues where the leader WAS tau seconds ago.
    const bool ctrl_r = io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_R);
    if (ctrl_r && !m_ctrl_r_prev) {
        if (!m_curves.empty()) {
            // Ensure the leader has history recording enabled.
            if (m_curves[0].history() == nullptr) {
                // capacity covers tau * 120fps + 50% margin
                const std::size_t cap =
                    static_cast<std::size_t>(
                        std::ceil(m_dp_params.tau * 120.f * 1.5f)) + 256;
                m_curves[0].enable_history(cap, 1.f / 120.f);
            }

            // Spawn the chaser offset from the leader's current head.
            const Vec3 ref = m_curves[0].head_world();
            const f32 ang  = static_cast<f32>(m_dp_count) * 1.1f + 0.3f;
            const f32 sx   = std::clamp(ref.x + 2.f * std::cos(ang),
                                        m_surface->u_min() + 0.5f,
                                        m_surface->u_max() - 0.5f);
            const f32 sy   = std::clamp(ref.y + 2.f * std::sin(ang),
                                        m_surface->v_min() + 0.5f,
                                        m_surface->v_max() - 0.5f);
            const u32 slot = m_chaser_count % AnimatedCurve::MAX_SLOTS;
            // The delay-pursuit equation holds a non-owning pointer to the
            // leader's HistoryBuffer.  The leader is m_curves[0] and must
            // not be erased while this chaser lives.
            m_curves.push_back(
                AnimatedCurve::with_equation(
                    sx, sy,
                    AnimatedCurve::Role::Chaser, slot,
                    m_surface.get(),
                    std::make_unique<ndde::sim::DelayPursuitEquation>(
                        m_curves[0].history(), m_surface.get(), m_dp_params),
                    &m_milstein));
            ++m_chaser_count;
            ++m_dp_count;
            // No pre-warm: the chaser needs the leader to accumulate history
            // before it can start pursuing, so it spawns cold.
        }
    }
    m_ctrl_r_prev = ctrl_r;

    const bool ctrl_q = io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_Q);
    if (ctrl_q && !m_ctrl_q_prev) {
        m_debug_open = !m_debug_open;
        m_coord_debug.visible() = m_debug_open;
    }
    m_ctrl_q_prev = ctrl_q;

    // Ctrl+H: toggle the hotkey reference panel.
    const bool ctrl_h = io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_H);
    if (ctrl_h && !m_ctrl_h_prev)
        m_hotkey_panel_open = !m_hotkey_panel_open;
    m_ctrl_h_prev = ctrl_h;
}

// ── canvas_mvp_3d ─────────────────────────────────────────────────────────────

Mat4 SurfaceSimScene::canvas_mvp_3d(const ImVec2& cpos, const ImVec2& csz) const noexcept {
    const Vec2 sw_sz = m_api.viewport_size();
    const f32  sw    = sw_sz.x > 0.f ? sw_sz.x : 1.f;
    const f32  sh    = sw_sz.y > 0.f ? sw_sz.y : 1.f;
    const f32  cw    = csz.x   > 0.f ? csz.x   : 1.f;
    const f32  ch    = csz.y   > 0.f ? csz.y   : 1.f;
    const f32  cx    = cpos.x;
    const f32  cy    = cpos.y;

    const f32 sx = cw / sw;
    const f32 sy = ch / sh;
    const f32 bx =  2.f*cx/sw + sx - 1.f;
    const f32 by = -(2.f*cy/sh + sy - 1.f);

    Mat4 remap(0.f);
    remap[0][0] = sx;
    remap[1][1] = sy;
    remap[2][2] = 1.f;
    remap[3][0] = bx;
    remap[3][1] = by;
    remap[3][3] = 1.f;

    const f32  aspect = cw / ch;
    const f32  dist   = m_vp3d.base_extent / m_vp3d.zoom * 3.f;
    const f32  ex     = m_vp3d.pan_x + dist*std::cos(m_vp3d.pitch)*std::sin(m_vp3d.yaw);
    const f32  ey     = m_vp3d.pan_y + dist*std::sin(m_vp3d.pitch);
    const f32  ez     =                dist*std::cos(m_vp3d.pitch)*std::cos(m_vp3d.yaw);
    const Mat4 proj   = glm::perspective(glm::radians(45.f), aspect, 0.01f, 500.f);
    const Mat4 view   = glm::lookAt(
        glm::vec3(ex, ey, ez),
        glm::vec3(m_vp3d.pan_x, m_vp3d.pan_y, 0.f),
        glm::vec3(0.f, 1.f, 0.f));

    return remap * proj * view;
}

// ── draw_simulation_panel ─────────────────────────────────────────────────────

// ── draw_hotkey_panel ─────────────────────────────────────────────────────────
void SurfaceSimScene::draw_hotkey_panel() {
    if (!m_hotkey_panel_open) return;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        { vp->WorkPos.x + vp->WorkSize.x - 320.f, vp->WorkPos.y + 40.f },
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({ 300.f, 0.f }, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.88f);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize;

    if (!ImGui::Begin("Hotkeys  [Ctrl+H]", &m_hotkey_panel_open, flags)) {
        ImGui::End();
        return;
    }

    // Two-column layout: key on the left, description on the right.
    auto row = [](const char* key, const char* desc) {
        ImGui::TextDisabled("%s", key);
        ImGui::SameLine(90.f);
        ImGui::TextUnformatted(desc);
    };

    ImGui::SeparatorText("Spawn particles");
    row("Ctrl+L", "Leader particle  (blue)");
    row("Ctrl+C", "Chaser particle  (red)");
    row("Ctrl+B", "Brownian particle  (Milstein)");
    row("Ctrl+R", "Delay-pursuit chaser");

    ImGui::SeparatorText("Overlays");
    row("Ctrl+F", "Frenet frame  (T, N, B)");
    row("Ctrl+D", "Surface frame  (Dx, Dy)");
    row("Ctrl+N", "Normal plane patch");
    row("Ctrl+T", "Torsion ribbon");

    ImGui::SeparatorText("Simulation");
    row("Ctrl+P", "Pause / unpause");

    ImGui::SeparatorText("Panels");
    row("Ctrl+Q", "Coordinate debug");
    row("Ctrl+H", "This panel");

    ImGui::Spacing();
    ImGui::TextDisabled("Ctrl+H or close button to dismiss.");
    ImGui::End();
}

// ── draw_simulation_panel ───────────────────────────────────────────────────────
void SurfaceSimScene::draw_simulation_panel() {
    ImGui::SetNextWindowPos(ImVec2(20.f, 20.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(310.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.88f);
    ImGui::Begin("Simulation");

    ImGui::SeparatorText("Surface");
    {
        // Surface selector: radio buttons, one per type.
        // Switching calls swap_surface() which rebuilds m_surface and resets
        // all particle positions into the new domain.
        int sel = static_cast<int>(m_surface_type);
        bool changed = false;
        changed |= ImGui::RadioButton("Gaussian", &sel, 0);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Torus",    &sel, 1);
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Ripple",   &sel, 2);
        if (changed)
            swap_surface(static_cast<SurfaceType>(sel));

        if (m_surface_type == SurfaceType::Torus) {
            bool tp = false;
            tp |= ImGui::SliderFloat("R##torus", &m_torus_R, 0.5f, 5.f, "%.2f");
            tp |= ImGui::SliderFloat("r##torus", &m_torus_r, 0.1f,
                                     m_torus_R - 0.05f, "%.2f");
            if (tp) swap_surface(SurfaceType::Torus);
        }

        if (m_surface_type == SurfaceType::GaussianRipple) {
            // Live parameter sliders -- changes sync to the active surface.
            auto& p  = m_ripple_params;
            bool  rp = false;
            rp |= ImGui::SliderFloat("Amplitude##r",  &p.amplitude,  0.05f, 2.f,  "%.2f");
            rp |= ImGui::SliderFloat("Damping##r",    &p.damping,    0.05f, 2.f,  "%.3f");
            rp |= ImGui::SliderFloat("Wavelength##r", &p.wavelength, 0.3f,  5.f,  "%.2f");
            rp |= ImGui::SliderFloat("Speed##r",      &p.speed,      0.1f,  6.f,  "%.2f");
            rp |= ImGui::SliderFloat("Sigma##r",      &p.sigma,      0.3f,  6.f,  "%.2f");
            if (rp) {
                // Sync new params into the live surface without rebuilding.
                if (auto* gr = dynamic_cast<GaussianRipple*>(m_surface.get()))
                    gr->params() = p;
            }
            ImGui::TextDisabled("Epicentre: (%.2f, %.2f)  t=%.2f",
                p.epicentre_u, p.epicentre_v, m_sim_time);
            ImGui::SameLine();
            if (ImGui::SmallButton("Re-trigger")) {
                // Re-trigger at the head of particle 0
                const Vec3 h = m_curves.empty() ? Vec3{0,0,0} : m_curves[0].head_world();
                p.epicentre_u = h.x;  p.epicentre_v = h.y;
                if (auto* gr = dynamic_cast<GaussianRipple*>(m_surface.get()))
                    gr->set_epicentre(h.x, h.y);
                m_sim_time = 0.f;
            }
        }
    }

    ImGui::SeparatorText("Curve");
    ImGui::Checkbox("Paused", &m_sim_paused);
    ImGui::SameLine();
    if (ImGui::Button("Reset") && !m_curves.empty())
        m_curves[0].reset();
    ImGui::SliderFloat("Speed##sim",  &m_sim_speed,   0.1f, 5.f, "%.2f");
    ImGui::SliderFloat("Arrow scale", &m_frame_scale, 0.05f,0.8f,"%.2f");
    int gl = static_cast<int>(m_grid_lines);
    if (ImGui::SliderInt("Grid lines", &gl, 8, 60)) {
        m_grid_lines = static_cast<u32>(gl);
        m_wireframe_dirty = true;
    }

    ImGui::SeparatorText("Frenet frame  [Ctrl+F]");
    ImGui::BeginDisabled(!m_show_frenet);
    ImGui::Checkbox("T (tangent)",  &m_show_T);
    ImGui::SameLine();
    ImGui::Checkbox("N (normal)",   &m_show_N);
    ImGui::SameLine();
    ImGui::Checkbox("B (binormal)", &m_show_B);
    ImGui::Checkbox("Osc. circle",  &m_show_osc);
    ImGui::EndDisabled();
    ImGui::Checkbox("Contour lines",&m_show_contours);

    // ── Overlays ──────────────────────────────────────────────────────────────
    ImGui::SeparatorText("Overlays");
    ImGui::Checkbox("Frenet frame  [Ctrl+F]",   &m_show_frenet);
    ImGui::Checkbox("Surface frame [Ctrl+D]",   &m_show_dir_deriv);
    ImGui::Checkbox("Normal plane  [Ctrl+N]",   &m_show_normal_plane);
    ImGui::Checkbox("Torsion ribbon [Ctrl+T]",  &m_show_torsion);

    ImGui::SeparatorText("Brownian motion  [Ctrl+B]");
    {
        // Brownian motion parameters (used when Ctrl+B spawns a new particle).
        // Each Brownian particle owns its BrownianMotion equation; these sliders
        // set the params for the NEXT spawned particle only.
        // Existing Brownian particles keep their own params.
        auto& p = m_bm_params;
        ImGui::SliderFloat("Sigma##bm",         &p.sigma,          0.01f, 2.f,   "%.3f");
        ImGui::SliderFloat("Drift##bm",          &p.drift_strength, -1.f,  1.f,  "%.3f");
        if (ImGui::SmallButton("Spawn Brownian [Ctrl+B]")) {
            // Same logic as hotkey, duplicated for the button.
            const Vec3 ref = m_curves.empty()
                ? Vec3{0.f, 0.f, 0.f} : m_curves[0].head_world();
            const f32 ang = static_cast<f32>(m_chaser_count + m_leader_count) * 0.7f + 1.0f;
            const f32 sx  = std::clamp(ref.x + 1.8f*std::cos(ang),
                                       m_surface->u_min()+0.5f, m_surface->u_max()-0.5f);
            const f32 sy  = std::clamp(ref.y + 1.8f*std::sin(ang),
                                       m_surface->v_min()+0.5f, m_surface->v_max()-0.5f);
            m_curves.push_back(
                AnimatedCurve::with_equation(
                    sx, sy,
                    AnimatedCurve::Role::Chaser,
                    m_chaser_count % AnimatedCurve::MAX_SLOTS,
                    m_surface.get(),
                    std::make_unique<ndde::sim::BrownianMotion>(p),
                    &m_milstein));
            ++m_chaser_count;
            for (int i = 0; i < 60; ++i)
                m_curves.back().advance(1.f/60.f, m_sim_speed);
        }
        ImGui::TextDisabled("Milstein integrator (strong order 1.0)");
    }

    ImGui::SeparatorText("Delay Pursuit  [Ctrl+R]");
    {
        // Delay-pursuit parameters.
        // Ctrl+R spawns a chaser targeting where curve 0 WAS tau seconds ago.
        // IMPORTANT: the chaser's history pointer targets m_curves[0].
        // Do not remove curve 0 while delay-pursuit chasers are active.
        auto& p = m_dp_params;
        ImGui::SliderFloat("Tau (delay)##dp",   &p.tau,           0.1f, 10.f, "%.2f s");
        ImGui::SliderFloat("Speed##dp",          &p.pursuit_speed, 0.1f,  3.f, "%.2f");
        ImGui::SliderFloat("Noise sigma##dp",    &p.noise_sigma,   0.f,   1.f, "%.3f");
        if (ImGui::SmallButton("Spawn Pursuer [Ctrl+R]")) {
            if (!m_curves.empty()) {
                if (m_curves[0].history() == nullptr) {
                    const std::size_t cap =
                        static_cast<std::size_t>(
                            std::ceil(p.tau * 120.f * 1.5f)) + 256;
                    m_curves[0].enable_history(cap, 1.f / 120.f);
                }
                const Vec3 ref = m_curves[0].head_world();
                const f32 ang  = static_cast<f32>(m_dp_count) * 1.1f + 0.3f;
                const f32 sx   = std::clamp(ref.x + 2.f * std::cos(ang),
                                            m_surface->u_min()+0.5f, m_surface->u_max()-0.5f);
                const f32 sy   = std::clamp(ref.y + 2.f * std::sin(ang),
                                            m_surface->v_min()+0.5f, m_surface->v_max()-0.5f);
                m_curves.push_back(
                    AnimatedCurve::with_equation(
                        sx, sy,
                        AnimatedCurve::Role::Chaser,
                        m_chaser_count % AnimatedCurve::MAX_SLOTS,
                        m_surface.get(),
                        std::make_unique<ndde::sim::DelayPursuitEquation>(
                            m_curves[0].history(), m_surface.get(), p),
                        &m_milstein));
                ++m_chaser_count;  ++m_dp_count;
            }
        }
        if (m_curves.empty() || m_curves[0].history() == nullptr)
            ImGui::TextDisabled("(leader has no history yet -- spawn first)");
        else {
            const auto* h = m_curves[0].history();
            const float window = h->newest_t() - h->oldest_t();
            ImGui::TextDisabled("History: %.1fs window  %zu records",
                window, h->size());
            if (window < p.tau)
                ImGui::TextColored({1.f,0.8f,0.1f,1.f},
                    "Warming up: %.1f/%.1fs", window, p.tau);
        }
    }

    // ── Torsion readout (live, appears under toggle when active) ──────────────
    // Geometric intuition:
    //   Torsion tau measures how fast B rotates about T: dB/ds = -tau * N.
    //   tau > 0: right-hand corkscrew (positive twist about T)
    //   tau < 0: left-hand corkscrew  (negative twist)
    //   tau = 0: curve is instantaneously planar (B constant => no twist)
    //   Formula: tau = (p'xp'').p''' / |p'xp''|^2   (Differentiator::torsion)
    if (m_show_torsion && !m_curves.empty() && m_curves[0].has_trail()) {
        const AnimatedCurve& c0 = m_curves[0];
        const u32 hi = c0.trail_size() > 2 ? c0.trail_size()-2 : 0;
        const FrenetFrame fr = c0.frenet_at(hi);
        const f32 tau = fr.tau;

        const ImVec4 tau_col =
            tau >  1e-4f ? ImVec4(1.f,  0.55f, 0.1f, 1.f)   // orange: right twist
          : tau < -1e-4f ? ImVec4(0.45f,0.8f,  1.f,  1.f)   // cyan:   left twist
                         : ImVec4(0.7f, 0.7f,  0.7f, 1.f);  // grey:   planar
        ImGui::Indent(8.f);
        // \xcf\x84 = UTF-8 tau
        ImGui::TextColored(tau_col, "\xcf\x84 = %+.6f", tau);
        ImGui::SameLine();
        if      (tau >  1e-4f) ImGui::TextDisabled("(right twist)");
        else if (tau < -1e-4f) ImGui::TextDisabled("(left twist)");
        else                   ImGui::TextDisabled("(planar)");
        if (fr.kappa > 1e-5f) {
            // |tau/kappa| = 1 for a circular helix — useful shape discriminant
            // \xcf\x84 = tau, \xce\xba = kappa
            ImGui::TextDisabled("|\xcf\x84/\xce\xba| = %.4f", std::abs(tau) / fr.kappa);
        }
        ImGui::Unindent(8.f);
    }

    ImGui::Separator();
    ImGui::TextDisabled("Particles: %zu  (L=%u  C=%u)  [Ctrl+L leader  Ctrl+C chaser  Ctrl+B brownian]",
        m_curves.size(), m_leader_count, m_chaser_count);
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear all")) {
        m_curves.clear();
        m_leader_count = 0;
        m_chaser_count = 0;
        m_curves.emplace_back(-4.5f, -4.0f, AnimatedCurve::Role::Leader, 0u, m_surface.get(), &m_equation, &m_integrator);
        m_leader_count = 1;
    }

    ImGui::SeparatorText("At head  (particle 0)");
    if (!m_curves.empty() && m_curves[0].has_trail()) {
        const AnimatedCurve& c0 = m_curves[0];
        const u32 hi = c0.trail_size() > 2 ? c0.trail_size()-2 : 0;
        const FrenetFrame fr = c0.frenet_at(hi);
        const Vec3 hp = c0.head_world();
        // Surface-aware height readout.
        if (m_surface_type == SurfaceType::Gaussian)
            ImGui::TextDisabled("f(x,y) = %.4f", m_surface->evaluate(hp.x, hp.y, m_sim_time).z);
        else
            ImGui::TextDisabled("p = (%.3f, %.3f, %.3f)",
                m_surface->evaluate(hp.x, hp.y, m_sim_time).x,
                m_surface->evaluate(hp.x, hp.y, m_sim_time).y,
                m_surface->evaluate(hp.x, hp.y, m_sim_time).z);
        ImGui::TextColored(ImVec4(1.f,0.5f,0.1f,1.f),
            "T (%.3f, %.3f, %.3f)", fr.T.x, fr.T.y, fr.T.z);
        ImGui::TextColored(ImVec4(0.2f,1.f,0.4f,1.f),
            "N (%.3f, %.3f, %.3f)", fr.N.x, fr.N.y, fr.N.z);
        ImGui::TextColored(ImVec4(0.3f,0.6f,1.f,1.f),
            "B (%.3f, %.3f, %.3f)", fr.B.x, fr.B.y, fr.B.z);
        ImGui::TextDisabled("kappa = %.5f  tau = %.5f", fr.kappa, fr.tau);
        const f32 osc_r = fr.kappa > 1e-5f ? 1.f/fr.kappa : 0.f;
        ImGui::TextDisabled("osc. radius = %.4f", osc_r);
        const f32 K = m_surface->gaussian_curvature(hp.x, hp.y, m_sim_time);
        const f32 H = m_surface->mean_curvature(hp.x, hp.y, m_sim_time);
        ImGui::TextDisabled("K = %.5f  H = %.5f", K, H);
        const SurfaceFrame sf = make_surface_frame(*m_surface, hp.x, hp.y, m_sim_time, &fr);
        ImGui::Separator();
        ImGui::TextDisabled("E=%.4f  F=%.4f  G=%.4f", sf.E, sf.F, sf.G);
        ImGui::TextDisabled("\xce\xba_n=%.5f  \xce\xba_g=%.5f", sf.kappa_n, sf.kappa_g);
        const f32 k2    = fr.kappa*fr.kappa;
        const f32 check = sf.kappa_n*sf.kappa_n + sf.kappa_g*sf.kappa_g;
        const bool ok   = std::abs(k2-check) < 1e-4f || k2 < 1e-8f;
        ImGui::TextColored(ok ? ImVec4(.4f,1.f,.4f,1.f) : ImVec4(1.f,.3f,.3f,1.f),
            "\xce\xba\xc2\xb2=\xce\xba_n\xc2\xb2+\xce\xba_g\xc2\xb2 %s", ok ? "\xe2\x9c\x93" : "!");
    }

    ImGui::SeparatorText("3D camera");
    // Surface display mode
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
            m_wireframe_dirty = true;  // ensure filled cache is built if switching to it
        }
        // Curvature scale -- only meaningful for Filled/Both
        if (m_surface_display != SurfaceDisplay::Wireframe) {
            if (ImGui::SliderFloat("K scale##curv", &m_curv_scale, 0.01f, 20.f, "%.2f"))
                m_wireframe_dirty = true;  // recolour the filled mesh
            ImGui::SameLine();
            ImGui::TextDisabled("curvature range");
        }
    }
    // Grid density slider -- controls wireframe smoothness.
    // Vertex count = 4*N*(N+1). N=64->~17k, N=128->~66k, N=256->~262k.
    // CPU evaluation is the limit at N>200 on complex surfaces.
    {
        int gl = static_cast<int>(m_grid_lines);
        if (ImGui::SliderInt("Grid lines##surface", &gl, 8, 256))
            m_grid_lines = static_cast<u32>(std::max(gl, 8));
        ImGui::SameLine();
        ImGui::TextDisabled("~%uk verts",
            (4u * m_grid_lines * (m_grid_lines + 1u)) / 1000u);
    }
    ImGui::SliderFloat("Yaw",   &m_vp3d.yaw,   -std::numbers::pi_v<f32>, std::numbers::pi_v<f32>, "%.2f");
    ImGui::SliderFloat("Pitch", &m_vp3d.pitch,  -1.5f, 1.5f, "%.2f");
    ImGui::SliderFloat("Zoom##3d", &m_vp3d.zoom, 0.1f, 5.f, "%.2f");
    if (ImGui::Button("Reset 3D")) m_vp3d.reset();
    ImGui::SameLine();
    if (ImGui::Button("Reset 2D")) m_vp2d.reset();

    ImGui::SeparatorText("Debug");
    if (ImGui::Button("Coord Debug (Ctrl+Q)"))
        m_debug_open = !m_debug_open;
    ImGui::SameLine();
    if (ImGui::Button("Perf")) m_perf.visible() = !m_perf.visible();
    const auto& s = m_api.debug_stats();
    const ImVec4 fc = s.fps >= 55.f ? ImVec4(0.4f,1.f,0.4f,1.f)
                    : s.fps >= 30.f ? ImVec4(1.f,0.8f,0.f,1.f)
                                    : ImVec4(1.f,0.3f,0.3f,1.f);
    ImGui::SameLine();
    ImGui::TextColored(fc, "%.0f fps", s.fps);

    ImGui::End();
}

// ── draw_surface_3d_window ────────────────────────────────────────────────────

void SurfaceSimScene::draw_surface_3d_window() {
    ImGui::SetNextWindowPos(ImVec2(330.f, 20.f),  ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(750.f, 660.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
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

    // Dispatch based on surface display mode.
    // Both: filled first (back to front), wireframe on top at reduced opacity.
    switch (m_surface_display) {
        case SurfaceDisplay::Wireframe:
            submit_wireframe_3d(mvp3d);
            break;
        case SurfaceDisplay::Filled:
            submit_filled_3d(mvp3d);
            break;
        case SurfaceDisplay::Both:
            submit_filled_3d(mvp3d);
            submit_wireframe_3d(mvp3d);   // drawn on top; alpha blended by the renderer
            break;
    }

    for (u32 ci = 0; ci < static_cast<u32>(m_curves.size()); ++ci) {
        const AnimatedCurve& c = m_curves[ci];
        if (!c.has_trail()) continue;
        submit_trail_3d(c, mvp3d);
        submit_head_dot_3d(c, mvp3d);

        // Overlays on the snapped curve, or curve 0 when nothing is snapped.
        const bool is_active = (m_snap_on_curve && m_snap_curve == static_cast<int>(ci))
                            || (!m_snap_on_curve && ci == 0);
        if (!is_active) continue;

        const u32 sidx = (m_snap_on_curve && m_snap_curve == static_cast<int>(ci)
                          && m_snap_idx >= 0)
            ? static_cast<u32>(m_snap_idx)
            : (c.trail_size() > 2 ? c.trail_size()-2 : 0);

        if (m_show_frenet) {
            submit_frenet_3d(c, sidx, mvp3d);
            if (m_show_osc) submit_osc_circle_3d(c, sidx, mvp3d);
        }
        if (m_show_dir_deriv)    submit_surface_frame_3d(c, sidx, mvp3d);
        if (m_show_normal_plane) submit_normal_plane_3d(c, sidx, mvp3d);
        if (m_show_torsion)      submit_torsion_3d(c, sidx, mvp3d);  // NEW
    }

    // Text overlay
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddText(ImVec2(cpos.x+8, cpos.y+6),
        IM_COL32(200,200,200,180),
        "Right-drag: orbit   Scroll: zoom   Ctrl+L: add particle");

    // Pause indicator -- prominent so the user can't miss it.
    if (m_sim_paused) {
        dl->AddText(ImVec2(cpos.x+8, cpos.y+24),
            IM_COL32(255, 210, 60, 240),
            "PAUSED  [Ctrl+P to resume]");
    }

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
// ImDrawList overlay — kept for the primary-window fallback.
// The second OS window gets Vulkan geometry via submit_contour_second_window().

void SurfaceSimScene::draw_contour_2d_window() {
    // Not called from on_frame() — the second window handles 2D rendering.
    // Kept compiled so the method exists for potential future use.
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
                if (d < best) {
                    best = d;
                    m_snap_idx   = static_cast<int>(i);
                    m_snap_curve = static_cast<int>(ci);
                }
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
                if (d < best) {
                    best = d;
                    m_snap_idx   = static_cast<int>(i);
                    m_snap_curve = static_cast<int>(ci);
                }
            }
        }
    }

    if (m_snap_idx >= 0) m_snap_on_curve = true;
}

// ── submit_arrow ──────────────────────────────────────────────────────────────

void SurfaceSimScene::submit_arrow(Vec3 origin, Vec3 dir, Vec4 color,
                                    f32 length, const Mat4& mvp, Topology topo)
{
    auto slice = m_api.acquire(2);
    Vertex* v  = slice.vertices();
    v[0] = { origin,              color };
    v[1] = { origin + dir*length, color };
    m_api.submit(slice, topo, DrawMode::VertexColor, color, mvp);
}

// ── submit_wireframe_3d ───────────────────────────────────────────────────────

void SurfaceSimScene::rebuild_wireframe_cache_if_needed() {
    const bool resolution_changed = (m_grid_lines != m_cached_grid_lines);
    const bool time_varying       = m_surface->is_time_varying();

    // Static surface, same resolution, not explicitly dirtied: cache is valid.
    if (!m_wireframe_dirty && !resolution_changed && !time_varying)
        return;

    const u32 n = m_surface->wireframe_vertex_count(m_grid_lines, m_grid_lines);

    // High-watermark resize: never shrink.
    // When the user drags the grid slider down and back up, we reuse the
    // existing allocation without any heap activity.
    if (m_wireframe_cache.size() < static_cast<std::size_t>(n))
        m_wireframe_cache.resize(n);

    m_surface->tessellate_wireframe(
        std::span<Vertex>{ m_wireframe_cache.data(), n },
        m_grid_lines, m_grid_lines,
        m_sim_time);   // snapshot at current simulation time

    m_wireframe_vcount  = n;
    m_cached_grid_lines = m_grid_lines;
    m_wireframe_dirty   = false;
    // For time_varying surfaces: m_wireframe_dirty stays false here;
    // the is_time_varying() branch above ensures they rebuild every frame
    // without any external dirty-set.
}

// ── curvature_color ─────────────────────────────────────────────────────────────
// Maps Gaussian curvature K to a perceptually meaningful colour.
//
// Geometric intuition:
//   K > 0  -- elliptic region (sphere-like, positively curved)
//             The surface bends the same way in all directions.
//             Warm colour (amber -> red) reads as "convex tension".
//   K = 0  -- parabolic region (flat or cylindrical, one principal curvature is zero)
//             Mid-grey: neutral, no net Gaussian bending.
//   K < 0  -- hyperbolic region (saddle-like, negatively curved)
//             The surface curves opposite ways in perpendicular directions.
//             Cool colour (teal -> blue) reads as "saddle tension".
//
// This encoding is standard in differential geometry visualisation
// (e.g. Ken Brakke's Surface Evolver, Rhino Curvature Analysis).
//
// scale: the |K| value that saturates the colour (maps to full red or blue).
//         Larger scale = more colour contrast for low-curvature surfaces.
// Opacity: 0.82 -- slightly transparent so trails remain visible through
//          the filled surface.

// static
Vec4 SurfaceSimScene::curvature_color(float K, float scale) noexcept {
    // Normalise K to [-1, 1]
    const float t = std::clamp(K / (scale + 1e-9f), -1.f, 1.f);

    if (t >= 0.f) {
        // Positive K: grey(0) -> amber(0.5) -> red(1)
        //   at t=0:   (0.50, 0.50, 0.50)  -- mid grey
        //   at t=0.5: (0.92, 0.60, 0.12)  -- amber
        //   at t=1:   (0.85, 0.12, 0.08)  -- red
        const float r = 0.50f + t * 0.35f;
        const float g = 0.50f - t * 0.38f;
        const float b = 0.50f - t * 0.42f;
        return { r, g, b, 0.82f };
    } else {
        // Negative K: grey(0) -> teal(-0.5) -> blue(-1)
        //   at t=0:    (0.50, 0.50, 0.50)  -- mid grey
        //   at t=-0.5: (0.12, 0.68, 0.72)  -- teal
        //   at t=-1:   (0.10, 0.22, 0.85)  -- blue
        const float s = -t;  // s in [0,1]
        const float r = 0.50f - s * 0.40f;
        const float g = 0.50f + s * 0.18f - s * s * 0.46f;
        const float b = 0.50f + s * 0.35f;
        return { r, g, b, 0.82f };
    }
}

// ── rebuild_filled_cache_if_needed ───────────────────────────────────────────────
void SurfaceSimScene::rebuild_filled_cache_if_needed() {
    const bool resolution_changed = (m_grid_lines != m_cached_grid_lines);
    const bool time_varying       = m_surface->is_time_varying();

    if (!m_wireframe_dirty && !resolution_changed && !time_varying)
        return;

    const u32 N   = m_grid_lines;       // grid cells per axis
    const u32 n   = N * N * 6;         // 2 triangles * 3 vertices per cell

    if (m_filled_cache.size() < static_cast<std::size_t>(n))
        m_filled_cache.resize(n);

    const float u0 = m_surface->u_min(m_sim_time);
    const float u1 = m_surface->u_max(m_sim_time);
    const float v0 = m_surface->v_min(m_sim_time);
    const float v1 = m_surface->v_max(m_sim_time);
    const float du = (u1 - u0) / static_cast<float>(N);
    const float dv = (v1 - v0) / static_cast<float>(N);

    u32 idx = 0;
    for (u32 i = 0; i < N; ++i) {
        const float ua = u0 + static_cast<float>(i)   * du;
        const float ub = u0 + static_cast<float>(i+1) * du;
        for (u32 j = 0; j < N; ++j) {
            const float va = v0 + static_cast<float>(j)   * dv;
            const float vb = v0 + static_cast<float>(j+1) * dv;

            // Evaluate the four corners of this (u,v) cell
            const Vec3 p00 = m_surface->evaluate(ua, va, m_sim_time);
            const Vec3 p10 = m_surface->evaluate(ub, va, m_sim_time);
            const Vec3 p01 = m_surface->evaluate(ua, vb, m_sim_time);
            const Vec3 p11 = m_surface->evaluate(ub, vb, m_sim_time);

            // Sample curvature at the cell centre for colour.
            // One sample per cell is correct -- curvature varies smoothly
            // and this avoids 4x the gaussian_curvature() calls.
            const float uc = (ua + ub) * 0.5f;
            const float vc = (va + vb) * 0.5f;
            const float K  = m_surface->gaussian_curvature(uc, vc, m_sim_time);
            const Vec4  col = curvature_color(K, m_curv_scale);

            // Triangle 1: (00, 10, 11)
            m_filled_cache[idx++] = { p00, col };
            m_filled_cache[idx++] = { p10, col };
            m_filled_cache[idx++] = { p11, col };

            // Triangle 2: (00, 11, 01)
            m_filled_cache[idx++] = { p00, col };
            m_filled_cache[idx++] = { p11, col };
            m_filled_cache[idx++] = { p01, col };
        }
    }

    m_filled_vcount = idx;
    // Note: we do NOT update m_cached_grid_lines or m_wireframe_dirty here;
    // that is the wireframe cache's responsibility.  Both caches share the
    // same invalidation condition -- whichever is built second sees the flag
    // already cleared by whichever was built first.
    // In practice: wireframe is always built before filled (call order in
    // draw_surface_3d_window), so by the time rebuild_filled runs,
    // m_wireframe_dirty == false and the check above falls through only via
    // the resolution_changed or time_varying branches.
    //
    // To handle the case where filled is the ONLY thing being built
    // (mode == Filled), we also clear the grid-line cache sentinel here:
    m_cached_grid_lines = m_grid_lines;
    m_wireframe_dirty   = false;
}

void SurfaceSimScene::submit_wireframe_3d(const Mat4& mvp) {
    // Ensure the CPU cache is current.  For static surfaces this is a near-free
    // early-out (one bool check).  For deforming surfaces it re-tessellates
    // into a pre-allocated system-RAM buffer.
    rebuild_wireframe_cache_if_needed();
    if (m_wireframe_vcount == 0) return;

    // Acquire a GPU arena slice and memcpy the cached vertices in.
    // Hot path: one arena bump + one contiguous memcpy, no evaluate() calls.
    auto slice = m_api.acquire(m_wireframe_vcount);
    std::memcpy(slice.vertices(),
                m_wireframe_cache.data(),
                m_wireframe_vcount * sizeof(Vertex));
    m_api.submit(slice, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp);
}

// ── submit_filled_3d ───────────────────────────────────────────────────────
void SurfaceSimScene::submit_filled_3d(const Mat4& mvp) {
    rebuild_filled_cache_if_needed();
    if (m_filled_vcount == 0) return;

    auto slice = m_api.acquire(m_filled_vcount);
    std::memcpy(slice.vertices(),
                m_filled_cache.data(),
                m_filled_vcount * sizeof(Vertex));
    m_api.submit(slice, Topology::TriangleList, DrawMode::VertexColor, {1,1,1,1}, mvp);
}

// ── submit_trail_3d ───────────────────────────────────────────────────────────

void SurfaceSimScene::submit_trail_3d(const AnimatedCurve& c, const Mat4& mvp) {
    const u32 n = c.trail_vertex_count();
    if (n < 2) return;
    auto slice = m_api.acquire(n);
    c.tessellate_trail({slice.vertices(), n});
    m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor, {1,1,1,1}, mvp);
}

// ── submit_head_dot_3d ────────────────────────────────────────────────────────

void SurfaceSimScene::submit_head_dot_3d(const AnimatedCurve& c, const Mat4& mvp) {
    const Vec3 hp  = c.head_world();
    const Vec4 col = c.head_colour();
    auto slice = m_api.acquire(1);
    slice.vertices()[0] = { hp, col };
    m_api.submit(slice, Topology::LineStrip, DrawMode::UniformColor, col, mvp);
}

// ── submit_frenet_3d ──────────────────────────────────────────────────────────

void SurfaceSimScene::submit_frenet_3d(const AnimatedCurve& c, u32 trail_idx,
                                        const Mat4& mvp) {
    if (!c.has_trail() || trail_idx == 0) return;
    const FrenetFrame fr = c.frenet_at(trail_idx);
    const Vec3 o = c.trail_pt(trail_idx);
    if (m_show_T) submit_arrow(o, fr.T, {1.f,0.35f,0.05f,1.f}, m_frame_scale, mvp);
    if (m_show_N) submit_arrow(o, fr.N, {0.15f,1.f,0.3f,1.f},  m_frame_scale, mvp);
    if (m_show_B) submit_arrow(o, fr.B, {0.2f,0.5f,1.f,1.f},   m_frame_scale, mvp);
}

// ── submit_osc_circle_3d ─────────────────────────────────────────────────────

void SurfaceSimScene::submit_osc_circle_3d(const AnimatedCurve& c, u32 trail_idx,
                                             const Mat4& mvp) {
    if (!c.has_trail() || trail_idx == 0) return;
    const FrenetFrame fr = c.frenet_at(trail_idx);
    if (fr.kappa < 1e-5f) return;
    const f32  R      = 1.f / fr.kappa;
    const Vec3 o      = c.trail_pt(trail_idx);
    const Vec3 centre = o + fr.N * R;
    constexpr u32 SEG = 64;
    const Vec4 col = {0.7f,0.3f,1.f,0.75f};
    auto slice = m_api.acquire(SEG+1);
    Vertex* v  = slice.vertices();
    for (u32 i = 0; i <= SEG; ++i) {
        const f32 theta = (static_cast<f32>(i)/SEG)*2.f*std::numbers::pi_v<f32>;
        v[i] = { centre + R*(-std::cos(theta)*fr.N + std::sin(theta)*fr.T), col };
    }
    m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor, col, mvp);
}

// ── submit_surface_frame_3d ───────────────────────────────────────────────────
// Dx cyan, Dy magenta, D_T (directional derivative in T) yellow.

void SurfaceSimScene::submit_surface_frame_3d(const AnimatedCurve& c, u32 trail_idx,
                                               const Mat4& mvp) {
    if (!c.has_trail() || trail_idx == 0) return;
    const Vec3 p  = c.trail_pt(trail_idx);
    const SurfaceFrame sf = make_surface_frame(*m_surface, p.x, p.y, m_sim_time);
    const f32 scale = m_frame_scale;

    submit_arrow(p, glm::normalize(sf.Dx), {0.1f, 0.9f, 0.9f, 1.f},
                 std::clamp(glm::length(sf.Dx)*scale, 0.04f, 1.5f), mvp);
    submit_arrow(p, glm::normalize(sf.Dy), {0.9f, 0.1f, 0.9f, 1.f},
                 std::clamp(glm::length(sf.Dy)*scale, 0.04f, 1.5f), mvp);

    const FrenetFrame fr  = c.frenet_at(trail_idx);
    const Vec3 T_xy_raw   = Vec3{fr.T.x, fr.T.y, 0.f};
    if (glm::length(T_xy_raw) < 1e-6f) return;
    const Vec3 T_xy = glm::normalize(T_xy_raw);
    const Vec3 Tv   = T_xy.x * sf.Dx + T_xy.y * sf.Dy;
    if (glm::length(Tv) < 1e-6f) return;
    submit_arrow(p, glm::normalize(Tv), {1.f, 0.9f, 0.05f, 1.f},
                 std::clamp(glm::length(Tv)*scale, 0.04f, 1.5f), mvp);
}

// ── submit_normal_plane_3d ────────────────────────────────────────────────────
// Amber rectangle in the plane spanned by surface normal n and curve tangent T.

void SurfaceSimScene::submit_normal_plane_3d(const AnimatedCurve& c, u32 trail_idx,
                                              const Mat4& mvp) {
    if (!c.has_trail() || trail_idx == 0) return;
    const Vec3        p   = c.trail_pt(trail_idx);
    const FrenetFrame fr  = c.frenet_at(trail_idx);
    const SurfaceFrame sf = make_surface_frame(*m_surface, p.x, p.y, m_sim_time, &fr);

    const f32 osc_r  = (fr.kappa > 1e-5f) ? 1.f / fr.kappa : 2.f;
    const f32 half_T = std::clamp(osc_r * 0.3f * m_frame_scale, 0.04f, 1.2f);
    const f32 half_n = std::clamp(std::sqrt(sf.E) * m_frame_scale, 0.04f, 1.2f);

    const Vec3 a = fr.T     * half_T;
    const Vec3 b = sf.normal* half_n;
    const Vec3 corners[5] = { p-a-b, p+a-b, p+a+b, p-a+b, p-a-b };

    const Vec4 col = {0.9f, 0.6f, 0.1f, 0.85f};
    auto slice = m_api.acquire(5);
    Vertex* v  = slice.vertices();
    for (u32 i = 0; i < 5; ++i) v[i] = { corners[i], col };
    m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor, col, mvp);

    auto diag = m_api.acquire(2);
    diag.vertices()[0] = { p-a+b, {col.r, col.g, col.b, 0.5f} };
    diag.vertices()[1] = { p+a-b, {col.r, col.g, col.b, 0.5f} };
    m_api.submit(diag, Topology::LineList, DrawMode::VertexColor, col, mvp);
}

// ── submit_torsion_3d ─────────────────────────────────────────────────────────
// Geometric meaning:
//   Torsion tau = (p' x p'') . p''' / |p' x p''|^2  measures how fast the
//   osculating plane rotates about the tangent T as we walk along the curve.
//   Equivalently, dB/ds = -tau * N (third Frenet-Serret equation).
//
// Visualisation strategy:
//   A short "twist ribbon" in the osculating plane (spanned by T and N) that
//   fans from B toward -B proportionally to |tau|.
//   Colour: orange for tau > 0 (right-hand twist), cyan for tau < 0 (left).
//   Additionally, a signed dB/ds arrow along N (since dB/ds = -tau*N):
//     - it points in +N when tau < 0, and in -N when tau > 0.
//   Both pieces are off when tau is essentially zero (|tau| < 1e-4).

void SurfaceSimScene::submit_torsion_3d(const AnimatedCurve& c, u32 trail_idx,
                                         const Mat4& mvp) {
    if (!c.has_trail() || trail_idx == 0) return;
    const FrenetFrame fr = c.frenet_at(trail_idx);

    // Nothing meaningful to draw when curvature is zero (B is undefined)
    // or torsion is essentially zero (no twist to show).
    if (fr.kappa < 1e-5f) return;

    const Vec3 o = c.trail_pt(trail_idx);
    const f32  tau    = fr.tau;
    const f32  scl    = m_frame_scale;

    // ── Colour encoding: orange = right twist, cyan = left twist ─────────────
    const Vec4 col = tau > 0.f
        ? Vec4{1.f, 0.55f, 0.1f, 0.9f}    // orange: positive torsion
        : Vec4{0.3f, 0.8f, 1.f,  0.9f};   // cyan:   negative torsion

    // ── (1) dB/ds arrow — direction is -tau*N ────────────────────────────────
    // Frenet-Serret: dB/ds = -tau * N
    // So the arrow points along -N when tau > 0, along +N when tau < 0.
    // Length proportional to |tau|, clamped for visual clarity.
    const f32  dB_len  = std::clamp(std::abs(tau) * scl * 2.f, 0.02f, 1.5f);
    const Vec3 dB_dir  = fr.N * (tau > 0.f ? -1.f : 1.f);  // -tau*N normalised
    submit_arrow(o, dB_dir, col, dB_len, mvp);

    // ── (2) Twist fan in the osculating plane (T-N plane) ────────────────────
    // The fan sweeps from N toward B, representing the rotation of the
    // osculating plane about T.  The opening angle is:
    //   alpha = atan(|tau| / kappa) — the helix angle for a helix with this
    //   curvature and torsion ratio.  Clamped to at most pi/3 for clarity.
    if (std::abs(tau) < 1e-4f) return;  // truly planar — skip the fan

    const f32  helix_angle = std::atan(std::abs(tau) / fr.kappa);
    const f32  fan_angle   = std::clamp(helix_angle, 0.01f, std::numbers::pi_v<f32> / 3.f);
    const f32  fan_r       = scl * 0.7f;
    constexpr u32 FAN_SEG  = 20;

    // Fan vertices: origin + arc from N toward (tau>0: -B, tau<0: +B)
    // Convention: positive tau rotates the osculating plane in the T x N = B
    // direction, so the fan opens toward -B for tau > 0.
    const Vec3 side = tau > 0.f ? -fr.B : fr.B;

    auto fan = m_api.acquire(FAN_SEG + 2);
    Vertex* vf = fan.vertices();
    vf[0] = { o, col };   // hub at curve point
    for (u32 i = 0; i <= FAN_SEG; ++i) {
        const f32 a = (static_cast<f32>(i) / FAN_SEG) * fan_angle;
        // Rotate from N toward side by angle a in the osculating plane
        const Vec3 spoke = std::cos(a) * fr.N + std::sin(a) * side;
        vf[i + 1] = { o + spoke * fan_r, {col.r, col.g, col.b, 0.45f} };
    }
    m_api.submit(fan, Topology::TriangleList, DrawMode::VertexColor, col, mvp);

    // Fan outline (line strip) — sharper boundary
    auto outline = m_api.acquire(FAN_SEG + 2);
    Vertex* vo   = outline.vertices();
    vo[0] = { o, col };
    for (u32 i = 0; i <= FAN_SEG; ++i) {
        const f32 a    = (static_cast<f32>(i) / FAN_SEG) * fan_angle;
        const Vec3 sp  = std::cos(a) * fr.N + std::sin(a) * side;
        vo[i + 1] = { o + sp * fan_r, col };
    }
    m_api.submit(outline, Topology::LineStrip, DrawMode::VertexColor, col, mvp);
}

// ── submit_contour_second_window ──────────────────────────────────────────────
// Sends heatmap + contours + all particle trails to the second OS window.

void SurfaceSimScene::submit_contour_second_window() {
    const Vec2 sz2 = m_api.viewport_size2();
    if (sz2.x <= 0.f || sz2.y <= 0.f) return;

    // Ortho MVP for the 2D contour window.
    // Use the surface's u-domain half-extent as the view scale.
    const f32 domain = m_surface->u_max();
    const f32 aspect = sz2.x / sz2.y;
    const f32 half_y = domain / m_vp2d.zoom;
    const f32 half_x = half_y * aspect;
    const Mat4 mvp2  = glm::ortho(
        m_vp2d.pan_x - half_x, m_vp2d.pan_x + half_x,
        m_vp2d.pan_y - half_y, m_vp2d.pan_y + half_y,
        -10.f, 10.f);

    // ── Heatmap + contour lines (graph-surface specific) ──────────────────────
    // These renderings depend on height_color() and tessellate_contours() which
    // are properties of a height-field (z = f(x,y)) surface.
    // dynamic_cast returns non-null only when m_surface IS a GaussianSurface.
    // For other surfaces (torus, sphere) this block is silently skipped.
    // A generic heat/contour approach will replace this in a later step.
    const auto* gs = dynamic_cast<const GaussianSurface*>(m_surface.get());
    if (gs) {
        // ── Heatmap (triangle mesh) ──────────────────────────────────────────
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
        m_api.submit2(tr, Topology::TriangleList, DrawMode::VertexColor, {1,1,1,1}, mvp2);

        // ── Contour lines ────────────────────────────────────────────────────
        if (m_show_contours) {
            const u32 max_v = GaussianSurface::contour_max_vertices(100u, k_n_levels);
            auto cslice = m_api.acquire(max_v);
            const u32 actual = GaussianSurface::tessellate_contours(
                {cslice.vertices(),max_v}, 100u, k_levels, k_n_levels, {1,1,1,0.8f});
            if (actual > 0) {
                memory::ArenaSlice tr2=cslice; tr2.vertex_count=actual;
                m_api.submit2(tr2, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp2);
            }
        }
    } // end gs block

    // ── Trails — all particles ─────────────────────────────────────────────────
    for (const AnimatedCurve& c : m_curves) {
        const u32 nt = c.trail_size();
        if (nt < 2) continue;
        auto slice = m_api.acquire(nt);
        Vertex* vt = slice.vertices();
        for (u32 i = 0; i < nt; ++i) {
            const Vec3 pt = c.trail_pt(i);
            const f32  t  = static_cast<f32>(i)/static_cast<f32>(nt-1);
            vt[i] = { Vec3{pt.x,pt.y,0}, AnimatedCurve::trail_colour(c.role(), c.colour_slot(), t) };
        }
        m_api.submit2(slice, Topology::LineStrip, DrawMode::VertexColor, {1,1,1,1}, mvp2);
    }

    // ── Frenet + torsion overlays on active curve (curve 0 or snapped) ────────
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
                    m_api.submit2(s, Topology::LineList, DrawMode::VertexColor, col, mvp2);
                };
                if (m_show_T) arr({fr.T.x,fr.T.y,0},{1.f,0.35f,0.05f,1.f});
                if (m_show_N) arr({fr.N.x,fr.N.y,0},{0.15f,1.f,0.3f,1.f});
                if (m_show_osc && fr.kappa > 1e-5f) {
                    const f32 R = 1.f/fr.kappa;
                    constexpr u32 SEG = 64;
                    auto sc2 = m_api.acquire(SEG+1); Vertex* vc = sc2.vertices();
                    const Vec3 ctr = {o3.x+fr.N.x*R, o3.y+fr.N.y*R, 0};
                    for (u32 k=0; k<=SEG; ++k) {
                        const f32 th = (static_cast<f32>(k)/SEG)*2.f*std::numbers::pi_v<f32>;
                        vc[k] = { ctr+Vec3{std::cos(th)*R,std::sin(th)*R,0},
                                  {0.7f,0.3f,1.f,0.65f} };
                    }
                    m_api.submit2(sc2, Topology::LineStrip, DrawMode::VertexColor,
                                  {0.7f,0.3f,1.f,0.65f}, mvp2);
                }

                // ── 2D torsion indicator: dB/ds arrow projected onto the xy-plane ──
                // In the 2D contour view the z-axis is invisible, so we project
                // the dB/ds = -tau*N vector into the xy-plane.
                // The result is a short arrow along N_xy coloured by torsion sign.
                if (m_show_torsion && fr.kappa > 1e-5f && std::abs(fr.tau) > 1e-4f) {
                    const Vec4 tau_col = fr.tau > 0.f
                        ? Vec4{1.f, 0.55f, 0.1f, 0.9f}   // orange: right twist
                        : Vec4{0.3f, 0.8f, 1.f,  0.9f};  // cyan:   left twist
                    // dB/ds = -tau * N  =>  project to xy
                    const Vec3 dBds_xy = { -fr.tau * fr.N.x, -fr.tau * fr.N.y, 0.f };
                    arr(dBds_xy, tau_col);  // reuse arr lambda

                    // Label: small dot at tip coloured by sign
                    const f32 len = std::clamp(std::abs(fr.tau) * scl * 2.f, 0.02f, 1.5f);
                    const Vec3 tip_dir = glm::length(dBds_xy) > 1e-6f
                        ? glm::normalize(dBds_xy) : Vec3{0,1,0};
                    const Vec3 tip = o + tip_dir * len;
                    auto dot = m_api.acquire(1);
                    dot.vertices()[0] = { tip, tau_col };
                    m_api.submit2(dot, Topology::LineStrip, DrawMode::UniformColor,
                                  tau_col, mvp2);
                }
            }
        }
    }
}

} // namespace ndde
