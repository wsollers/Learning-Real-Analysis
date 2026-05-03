// app/Scene.cpp
// NOTE: All ImGui text strings use plain ASCII only.
// MSVC C2022: hex escape sequences like \xc2\xb2 are parsed as a single
// multi-byte escape (0xc2b2 = 50354) which exceeds char range, even with
// /utf-8.  UTF-8 characters used elsewhere in the codebase (delta, epsilon,
// Greek letters for labels) are in separate string literals where the
// escape is the entire content — those are safe.  Panel description strings
// use ASCII math notation instead.
#include "app/Scene.hpp"
#include <imgui.h>
#include <format>
#include <cmath>
#include <numbers>
#include <limits>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ndde {

// ═══════════════════════════════════════════════════════════════════════════════
// ConicEntry
// ═══════════════════════════════════════════════════════════════════════════════

void ConicEntry::rebuild() {
    needs_rebuild = false;

    if (type == 0) {
        conic = std::make_unique<math::Parabola>(par_a, par_b, par_c, par_tmin, par_tmax);
    } else if (type == 1) {
        const auto axis = (hyp_axis == 0)
            ? math::HyperbolaAxis::Horizontal
            : math::HyperbolaAxis::Vertical;
        conic = std::make_unique<math::Hyperbola>(hyp_a, hyp_b, hyp_h, hyp_k, axis, hyp_range);
    } else if (type == 2) {
        conic = std::make_unique<math::Helix>(hel_radius, hel_pitch, hel_tmin, hel_tmax);
    } else if (type == 3) {
        conic = std::make_unique<math::ParaboloidCurve>(pc_a, pc_theta, pc_tmin, pc_tmax);
    }

    snap_cache.clear();
    snap_cache3d.clear();
    if (!conic) return;

    // Types 2 (Helix) and 3 (ParaboloidCurve) are 3D — use snap_cache3d
    if (type == 2 || type == 3) {
        const float t0   = conic->t_min();
        const float step = (conic->t_max() - t0) / static_cast<float>(tessellation);
        snap_cache3d.reserve((tessellation + 1u) * 3u);
        for (u32 i = 0; i <= tessellation; ++i) {
            const auto p = conic->evaluate(t0 + static_cast<float>(i) * step);
            snap_cache3d.push_back(p.x);
            snap_cache3d.push_back(p.y);
            snap_cache3d.push_back(p.z);
        }
    } else if (type == 1 && hyp_two_branch) {
        auto* hyp = static_cast<math::Hyperbola*>(conic.get());
        const float t0   = -hyp_range;
        const float step = (2.f * hyp_range) / static_cast<float>(tessellation);
        snap_cache.reserve((tessellation + 1u) * 2u);
        for (u32 i = 0; i <= tessellation; ++i) {
            const auto p = hyp->eval_branch(t0 + static_cast<float>(i) * step, true);
            snap_cache.push_back({ p.x, p.y });
        }
        for (u32 i = 0; i <= tessellation; ++i) {
            const auto p = hyp->eval_branch(t0 + static_cast<float>(i) * step, false);
            snap_cache.push_back({ p.x, p.y });
        }
    } else {
        snap_cache.resize(tessellation + 1u);
        const float t0   = conic->t_min();
        const float step = (conic->t_max() - t0) / static_cast<float>(tessellation);
        for (u32 i = 0; i <= tessellation; ++i) {
            const auto p = conic->evaluate(t0 + static_cast<float>(i) * step);
            snap_cache[i] = { p.x, p.y };
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// SurfaceEntry
// ═══════════════════════════════════════════════════════════════════════════════

void SurfaceEntry::rebuild() {
    needs_rebuild = false;
    if (type == 0) {
        surface = std::make_unique<math::Paraboloid>(par_a, par_umax);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Scene
// ═══════════════════════════════════════════════════════════════════════════════

Scene::Scene(EngineAPI api) : m_api(std::move(api)) {
    m_vp.base_extent = m_axes_cfg.extent * 1.2f;
    add_parabola();
    add_hyperbola();
    add_helix();
    add_paraboloid();  // adds paraboloid surface + its meridional curve together
}

void Scene::add_parabola() {
    ConicEntry e;
    e.name         = std::format("Parabola {}", m_conics.size());
    e.type         = 0;
    e.color        = { 1.0f, 0.8f, 0.2f, 1.0f };
    e.tessellation = m_api.config().simulation.tessellation;
    e.par_a = -1.f; e.par_b = 0.f; e.par_c = 0.f;
    e.par_tmin = -5.f; e.par_tmax = 5.f;
    e.rebuild();
    m_conics.push_back(std::move(e));
}

void Scene::add_hyperbola() {
    ConicEntry e;
    e.name         = std::format("Hyperbola {}", m_conics.size());
    e.type         = 1;
    e.color        = { 0.4f, 0.8f, 1.0f, 1.0f };
    e.tessellation = m_api.config().simulation.tessellation;
    e.hyp_range    = 2.5f;
    e.rebuild();
    m_conics.push_back(std::move(e));
}

void Scene::add_helix() {
    ConicEntry e;
    e.name         = std::format("Helix {}", m_conics.size());
    e.type         = 2;
    e.color        = { 0.9f, 0.4f, 0.9f, 1.0f };
    e.tessellation = m_api.config().simulation.tessellation;
    e.hel_radius   = 1.f;
    e.hel_pitch    = 0.5f;
    e.hel_tmin     = 0.f;
    e.hel_tmax     = 4.f * std::numbers::pi_v<float>;
    e.enabled      = false;
    e.rebuild();
    m_conics.push_back(std::move(e));
}

void Scene::add_paraboloid() {
    SurfaceEntry s;
    s.name     = std::format("Paraboloid {}", m_surfaces.size());
    s.type     = 0;
    s.par_a    = 1.f;
    s.par_umax = 1.5f;
    s.u_lines  = 14;
    s.v_lines  = 20;
    s.color    = { 0.35f, 0.55f, 0.85f, 0.45f };
    s.enabled  = false;
    s.rebuild();
    m_surfaces.push_back(std::move(s));
    add_paraboloid_curve();
}

void Scene::add_paraboloid_curve() {
    ConicEntry e;
    e.name         = std::format("ParaboloidCurve {}", m_conics.size());
    e.type         = 3;
    e.color        = { 1.0f, 0.55f, 0.1f, 1.0f };
    e.tessellation = m_api.config().simulation.tessellation;
    e.pc_a         = 1.f;
    e.pc_theta     = 0.f;
    e.pc_tmin      = 0.f;
    e.pc_tmax      = 1.5f;
    e.enabled      = false;
    e.rebuild();
    m_conics.push_back(std::move(e));
}

// ── on_frame ──────────────────────────────────────────────────────────────────

void Scene::on_frame() {
    sync_viewport();
    update_camera();

    for (auto& e : m_conics) {
        if (e.type == 0 && e.conic) {
            const float margin = 1.0f;
            if (m_vp.left()  < e.par_tmin + margin ||
                m_vp.right() > e.par_tmax - margin) {
                e.par_tmin = m_vp.left()  - 1.0f;
                e.par_tmax = m_vp.right() + 1.0f;
                e.mark_dirty();
            }
        }
    }

    if (m_axes_cfg.is_3d) update_hover_3d();
    else                  update_hover();

    m_coord_debug.update(m_vp, m_hover, m_conics, m_api.viewport_size());

    draw_main_panel();
    m_analysis_panel.draw(m_hover, m_api);
    m_coord_debug.draw();
    m_perf_panel.draw(m_api.debug_stats());
    submit_interval_labels();

    submit_grid();
    submit_axes();
    submit_surfaces();
    submit_conics();

    if (m_axes_cfg.is_3d) {
        submit_epsilon_sphere();
        submit_frenet_frame();
        submit_osc_circle();
    } else {
        submit_epsilon_ball();
        submit_interval_lines();
        submit_secant_line();
        submit_tangent_line();
        submit_frenet_frame();
        submit_osc_circle();
    }
}

// ── sync_viewport ─────────────────────────────────────────────────────────────

void Scene::sync_viewport() {
    const Vec2 fb = m_api.viewport_size();
    m_vp.fb_w = fb.x;
    m_vp.fb_h = fb.y;

    const ImGuiIO& io = ImGui::GetIO();
    m_vp.dp_w = io.DisplaySize.x;
    m_vp.dp_h = io.DisplaySize.y;

    m_vp.base_extent = m_axes_cfg.extent * 1.2f;
}

// ── update_camera ─────────────────────────────────────────────────────────────

void Scene::update_camera() {
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    // Correct for ViewportsEnable: MousePos is absolute screen coords.
    const Vec2 wm = Viewport::screen_to_window(io.MousePos.x, io.MousePos.y);

    if (std::abs(io.MouseWheel) > 0.f)
        m_vp.zoom_toward(wm.x, wm.y, io.MouseWheel);

    if (io.MouseDelta.x == 0.f && io.MouseDelta.y == 0.f) return;

    const bool middle = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
    const bool alt_l  = io.KeyAlt && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    const bool right  = ImGui::IsMouseDown(ImGuiMouseButton_Right);

    if (m_axes_cfg.is_3d) {
        if (right || middle) m_vp.orbit(io.MouseDelta.x, io.MouseDelta.y);
    } else {
        if (middle || alt_l) m_vp.pan_by_pixels(io.MouseDelta.x, io.MouseDelta.y);
    }
}

// ── update_hover (2D) ─────────────────────────────────────────────────────────

void Scene::update_hover() {
    m_hover = HoverResult{};

    const ImGuiIO& io = ImGui::GetIO();
    if (io.MousePos.x < 0.f || io.MousePos.y < 0.f) return;
    if (ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered()) return;

    // When ViewportsEnable is active MousePos is absolute screen coords.
    // Convert to window-relative before passing to pixel_to_world.
    const Vec2 win_mouse = Viewport::screen_to_window(io.MousePos.x, io.MousePos.y);
    const Vec2  mw = m_vp.pixel_to_world(win_mouse.x, win_mouse.y);
    const float world_per_px = (m_vp.dp_h > 0.f)
        ? (m_vp.top() - m_vp.bottom()) / m_vp.dp_h : 0.01f;
    const float snap_r_world = m_analysis_panel.get_snap_px_radius() * world_per_px;

    float best_d  = snap_r_world;
    int   best_ci = -1, best_si = -1;
    float best_x = 0.f, best_y = 0.f;

    for (int ci = 0; ci < static_cast<int>(m_conics.size()); ++ci) {
        const auto& entry = m_conics[ci];
        if (!entry.enabled || entry.type == 2 || entry.type == 3) continue;
        if (entry.snap_cache.empty()) continue;
        for (int si = 0; si < static_cast<int>(entry.snap_cache.size()); ++si) {
            const auto [px, py] = entry.snap_cache[si];
            const float d = std::hypot(px - mw.x, py - mw.y);
            if (d < best_d) { best_d = d; best_ci = ci; best_si = si; best_x = px; best_y = py; }
        }
    }

    if (best_ci < 0) return;

    m_hover.hit       = true;
    m_hover.world_x   = best_x;
    m_hover.world_y   = best_y;
    m_hover.world_z   = 0.f;
    m_hover.curve_idx = best_ci;
    m_hover.snap_idx  = best_si;

    const auto& e_ref     = m_conics[best_ci];
    const auto& cache     = e_ref.snap_cache;
    const int   total     = static_cast<int>(cache.size());
    const int   branch_end = (e_ref.type == 1 && e_ref.hyp_two_branch)
        ? (best_si <= static_cast<int>(e_ref.tessellation)
            ? static_cast<int>(e_ref.tessellation) : total - 1)
        : total - 1;
    const int branch_start = (e_ref.type == 1 && e_ref.hyp_two_branch)
        ? (best_si <= static_cast<int>(e_ref.tessellation)
            ? 0 : static_cast<int>(e_ref.tessellation) + 1)
        : 0;

    const int   branch_i = best_si - branch_start;
    const int   branch_n = branch_end - branch_start;
    const float frac     = (branch_n > 0)
        ? static_cast<float>(branch_i) / static_cast<float>(branch_n) : 0.f;
    const float snap_t   = e_ref.conic->t_min() +
        frac * (e_ref.conic->t_max() - e_ref.conic->t_min());
    m_hover.snap_t = snap_t;

    const float secant_r = m_analysis_panel.get_epsilon_ball_radius();
    int li = best_si;
    while (li > branch_start && std::hypot(cache[li-1].first  - best_x,
                                            cache[li-1].second - best_y) <= secant_r) --li;
    int ri = best_si;
    while (ri < branch_end && std::hypot(cache[ri+1].first  - best_x,
                                          cache[ri+1].second - best_y) <= secant_r) ++ri;

    if (li != ri) {
        const auto [x0,y0] = cache[li]; const auto [x1,y1] = cache[ri];
        const float dx = x1-x0, dy = y1-y0;
        m_hover.has_secant = true;
        m_hover.secant_x0 = x0; m_hover.secant_y0 = y0;
        m_hover.secant_x1 = x1; m_hover.secant_y1 = y1;
        m_hover.slope      = (std::abs(dx) > 1e-9f) ? dy/dx : 0.f;
        m_hover.intercept  = y0 - m_hover.slope * x0;
    }

    if (e_ref.conic) {
        const auto& c  = *e_ref.conic;
        const Vec3  T  = c.unit_tangent(snap_t);
        const Vec3  N  = c.unit_normal(snap_t);
        const Vec3  B  = c.unit_binormal(snap_t);
        const Vec3  d1 = c.derivative(snap_t);
        m_hover.has_tangent   = true;
        m_hover.tangent_slope = (std::abs(T.x) > 1e-9f) ? T.y / T.x : 0.f;
        m_hover.T[0] = T.x; m_hover.T[1] = T.y; m_hover.T[2] = T.z;
        m_hover.N[0] = N.x; m_hover.N[1] = N.y; m_hover.N[2] = N.z;
        m_hover.B[0] = B.x; m_hover.B[1] = B.y; m_hover.B[2] = B.z;
        m_hover.kappa      = c.curvature(snap_t);
        m_hover.tau        = c.torsion(snap_t);
        m_hover.speed      = glm::length(d1);
        m_hover.osc_radius = (m_hover.kappa > 1e-8f) ? 1.f / m_hover.kappa : 0.f;
    }
}

// ── update_hover_3d ───────────────────────────────────────────────────────────

void Scene::update_hover_3d() {
    m_hover = HoverResult{};

    const ImGuiIO& io = ImGui::GetIO();
    if (!Viewport::mouse_valid(io.MousePos.x, io.MousePos.y)) return;
    if (ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered()) return;

    const Mat4  mvp     = m_vp.perspective_mvp();
    const float snap_px = m_analysis_panel.get_snap_px_radius();

    float best_screen_d = snap_px;
    int   best_ci = -1, best_si = -1;
    float best_x = 0.f, best_y = 0.f, best_z = 0.f;

    for (int ci = 0; ci < static_cast<int>(m_conics.size()); ++ci) {
        const auto& entry = m_conics[ci];
        if (!entry.enabled) continue;
        if ((entry.type == 2 || entry.type == 3) && !entry.snap_cache3d.empty()) {
            const int n = static_cast<int>(entry.snap_cache3d.size()) / 3;
            for (int si = 0; si < n; ++si) {
                const float px = entry.snap_cache3d[si*3+0];
                const float py = entry.snap_cache3d[si*3+1];
                const float pz = entry.snap_cache3d[si*3+2];
                const glm::vec4 clip = mvp * glm::vec4(px, py, pz, 1.f);
                if (clip.w <= 0.f) continue;
                const float sx = (clip.x / clip.w + 1.f) * 0.5f * m_vp.dp_w;
                const float sy = (1.f - clip.y / clip.w) * 0.5f * m_vp.dp_h;
                const float d  = std::hypot(sx - io.MousePos.x, sy - io.MousePos.y);
                if (d < best_screen_d) {
                    best_screen_d = d; best_ci = ci; best_si = si;
                    best_x = px; best_y = py; best_z = pz;
                }
            }
        }
    }

    if (best_ci < 0) return;

    m_hover.hit       = true;
    m_hover.world_x   = best_x;
    m_hover.world_y   = best_y;
    m_hover.world_z   = best_z;
    m_hover.curve_idx = best_ci;
    m_hover.snap_idx  = best_si;

    const auto& e_ref = m_conics[best_ci];
    if (e_ref.conic) {
        const int   n      = static_cast<int>(e_ref.snap_cache3d.size()) / 3;
        const float frac   = (n > 1)
            ? static_cast<float>(best_si) / static_cast<float>(n - 1) : 0.f;
        const float snap_t = e_ref.conic->t_min() +
            frac * (e_ref.conic->t_max() - e_ref.conic->t_min());
        m_hover.snap_t = snap_t;

        const auto& c  = *e_ref.conic;
        const Vec3  T  = c.unit_tangent(snap_t);
        const Vec3  N  = c.unit_normal(snap_t);
        const Vec3  B  = c.unit_binormal(snap_t);
        const Vec3  d1 = c.derivative(snap_t);
        m_hover.has_tangent   = true;
        m_hover.tangent_slope = 0.f;
        m_hover.T[0] = T.x; m_hover.T[1] = T.y; m_hover.T[2] = T.z;
        m_hover.N[0] = N.x; m_hover.N[1] = N.y; m_hover.N[2] = N.z;
        m_hover.B[0] = B.x; m_hover.B[1] = B.y; m_hover.B[2] = B.z;
        m_hover.kappa      = c.curvature(snap_t);
        m_hover.tau        = c.torsion(snap_t);
        m_hover.speed      = glm::length(d1);
        m_hover.osc_radius = (m_hover.kappa > 1e-8f) ? 1.f / m_hover.kappa : 0.f;
    }
}

// ── submit_surfaces ───────────────────────────────────────────────────────────

void Scene::submit_surfaces() {
    if (!m_axes_cfg.is_3d) return;
    const Mat4 mvp = m_vp.perspective_mvp();

    for (auto& entry : m_surfaces) {
        if (!entry.enabled) continue;
        if (entry.needs_rebuild) entry.rebuild();
        if (!entry.surface) continue;

        const u32   n     = entry.surface->wireframe_vertex_count(entry.u_lines, entry.v_lines);
        auto        slice = m_api.acquire(n);
        entry.surface->tessellate_wireframe({ slice.vertices(), n },
                                             entry.u_lines, entry.v_lines, 0.f, entry.color);
        m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineList, DrawMode::VertexColor, entry.color, mvp);
    }
}

// ── submit_epsilon_ball (2D) ──────────────────────────────────────────────────

void Scene::submit_epsilon_ball() {
    if (!m_hover.hit || !m_analysis_panel.show_epsilon_ball()) return;
    constexpr u32 seg = 64;
    const float r = m_analysis_panel.get_epsilon_ball_radius();
    const Vec4  col{ 0.95f, 0.95f, 0.15f, 0.9f };
    auto    slice = m_api.acquire(seg + 1);
    Vertex* v     = slice.vertices();
    for (u32 i = 0; i < seg; ++i) {
        const float theta = (static_cast<float>(i) / seg) * 2.f * std::numbers::pi_v<float>;
        v[i] = { Vec3{ m_hover.world_x + r*std::cos(theta),
                        m_hover.world_y + r*std::sin(theta), 0.f }, col };
    }
    v[seg] = v[0];
    m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineStrip, DrawMode::VertexColor, col, m_vp.ortho_mvp());
}

// ── submit_epsilon_sphere (3D) ────────────────────────────────────────────────

void Scene::submit_epsilon_sphere() {
    if (!m_hover.hit || !m_analysis_panel.show_epsilon_ball()) return;
    constexpr int lat = 8, lon = 8, seg = 48;
    const float r  = m_analysis_panel.get_epsilon_ball_radius();
    const float cx = m_hover.world_x, cy = m_hover.world_y, cz = m_hover.world_z;
    const Vec4  col{ 0.95f, 0.95f, 0.15f, 0.65f };
    const Mat4  mvp = m_vp.perspective_mvp();
    auto    slice = m_api.acquire(static_cast<u32>((lat + lon) * (seg + 1)));
    Vertex* v     = slice.vertices();
    u32     idx   = 0;
    for (int li = 1; li <= lat; ++li) {
        const float th = std::numbers::pi_v<float> * li / (lat + 1);
        const float st = std::sin(th), ct = std::cos(th);
        for (int s = 0; s <= seg; ++s) {
            const float ph = (static_cast<float>(s) / seg) * 2.f * std::numbers::pi_v<float>;
            v[idx++] = { Vec3{ cx + r*st*std::cos(ph), cy + r*ct, cz + r*st*std::sin(ph) }, col };
        }
    }
    for (int li = 0; li < lon; ++li) {
        const float ph = (static_cast<float>(li) / lon) * 2.f * std::numbers::pi_v<float>;
        const float cp = std::cos(ph), sp = std::sin(ph);
        for (int s = 0; s <= seg; ++s) {
            const float th = std::numbers::pi_v<float> * s / seg;
            const float st = std::sin(th), ct = std::cos(th);
            v[idx++] = { Vec3{ cx + r*st*cp, cy + r*ct, cz + r*st*sp }, col };
        }
    }
    memory::ArenaSlice trimmed = slice;
    trimmed.vertex_count = idx;
    m_api.submit_to(RenderTarget::Primary3D, trimmed, Topology::LineStrip, DrawMode::VertexColor, col, mvp);
}

// ── submit_frenet_frame ───────────────────────────────────────────────────────

void Scene::submit_frenet_frame() {
    if (!m_hover.hit || !m_hover.has_tangent) return;
    const float scale = m_analysis_panel.get_frame_scale();
    const Vec3  o{ m_hover.world_x, m_hover.world_y, m_hover.world_z };
    const Mat4  mvp = m_axes_cfg.is_3d ? m_vp.perspective_mvp() : m_vp.ortho_mvp();

    struct Arrow { const float* rgb; float dx,dy,dz; };
    const Arrow arrows[3] = {
        { m_analysis_panel.T_colour(), m_hover.T[0], m_hover.T[1], m_hover.T[2] },
        { m_analysis_panel.N_colour(), m_hover.N[0], m_hover.N[1], m_hover.N[2] },
        { m_analysis_panel.B_colour(), m_hover.B[0], m_hover.B[1], m_hover.B[2] },
    };
    const bool show[3] = {
        m_analysis_panel.show_unit_tangent(),
        m_analysis_panel.show_unit_normal(),
        m_analysis_panel.show_unit_binormal(),
    };
    for (int i = 0; i < 3; ++i) {
        if (!show[i]) continue;
        const Vec4 col{ arrows[i].rgb[0], arrows[i].rgb[1], arrows[i].rgb[2], 1.f };
        auto    slice = m_api.acquire(2);
        Vertex* v     = slice.vertices();
        v[0] = { o, col };
        v[1] = { Vec3{ o.x + arrows[i].dx*scale,
                        o.y + arrows[i].dy*scale,
                        o.z + arrows[i].dz*scale }, col };
        m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineList, DrawMode::VertexColor, col, mvp);
    }
}

// ── submit_osc_circle ─────────────────────────────────────────────────────────

void Scene::submit_osc_circle() {
    if (!m_hover.hit || !m_hover.has_tangent) return;
    if (!m_analysis_panel.show_curvature_circle()) return;
    if (m_hover.kappa < 1e-8f) return;

    const float R  = m_hover.osc_radius;
    const Vec3  o{ m_hover.world_x, m_hover.world_y, m_hover.world_z };
    const Vec3  N{ m_hover.N[0], m_hover.N[1], m_hover.N[2] };
    const Vec3  T{ m_hover.T[0], m_hover.T[1], m_hover.T[2] };
    const Vec3  centre = o + R * N;
    constexpr u32 seg = 64;
    const float* rgb = m_analysis_panel.osc_colour();
    const Vec4   col{ rgb[0], rgb[1], rgb[2], 0.85f };
    const Mat4   mvp = m_axes_cfg.is_3d ? m_vp.perspective_mvp() : m_vp.ortho_mvp();
    auto    slice = m_api.acquire(seg + 1);
    Vertex* v     = slice.vertices();
    for (u32 i = 0; i <= seg; ++i) {
        const float theta = (static_cast<float>(i) / seg) * 2.f * std::numbers::pi_v<float>;
        const Vec3  pt    = centre + R * (-std::cos(theta) * N + std::sin(theta) * T);
        v[i] = { pt, col };
    }
    m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineStrip, DrawMode::VertexColor, col, mvp);
}

// ── submit_interval_lines ─────────────────────────────────────────────────────

void Scene::submit_interval_lines() {
    if (!m_hover.hit || !m_analysis_panel.show_interval_lines()) return;
    const float cx = m_hover.world_x, cy = m_hover.world_y;
    const float eps = m_analysis_panel.get_epsilon_interval();
    const float* rgb = m_analysis_panel.interval_colour();
    const Vec4 col{ rgb[0], rgb[1], rgb[2], 0.8f };
    const Vec4 ctr{ rgb[0]*0.6f, rgb[1]*0.6f, rgb[2]*0.6f, 0.55f };
    auto    slice = m_api.acquire(24);
    Vertex* v     = slice.vertices();
    u32 idx = 0;
    auto push = [&](float x0,float y0,float x1,float y1,Vec4 c){
        v[idx++]={Vec3{x0,y0,0.f},c}; v[idx++]={Vec3{x1,y1,0.f},c};
    };
    push(cx-eps,cy-eps,cx+eps,cy-eps,col); push(cx-eps,cy+eps,cx+eps,cy+eps,col);
    push(cx-eps,cy-eps,cx-eps,cy+eps,col); push(cx+eps,cy-eps,cx+eps,cy+eps,col);
    push(cx,0.f,cx,cy,ctr); push(0.f,cy,cx,cy,ctr);
    const float tk = eps*0.4f;
    push(cx-tk,0.f,cx+tk,0.f,col); push(0.f,cy-tk,0.f,cy+tk,col);
    push(cx-eps,0.f,cx-eps,tk*2.f,col); push(cx+eps,0.f,cx+eps,tk*2.f,col);
    push(0.f,cy-eps,tk*2.f,cy-eps,col); push(0.f,cy+eps,tk*2.f,cy+eps,col);
    m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineList, DrawMode::VertexColor, col, m_vp.ortho_mvp());
}

// ── submit_interval_labels ────────────────────────────────────────────────────

void Scene::submit_interval_labels() {
    if (!m_hover.hit || !m_analysis_panel.show_interval_lines()) return;
    if (m_axes_cfg.is_3d) return;

    const float cx = m_hover.world_x, cy = m_hover.world_y;
    const float eps = m_analysis_panel.get_epsilon_interval();
    const float* rgb = m_analysis_panel.interval_colour();
    const ImVec4 label_col{ rgb[0], rgb[1], rgb[2], 0.95f };
    constexpr float PAD = 10.f;
    const bool snap_above_x = (cy > 0.f);
    const bool snap_right_y = (cx > 0.f);
    const float x_dy = snap_above_x ?  PAD : -PAD;
    const float y_dx = snap_right_y ? -PAD :  PAD;

    constexpr ImGuiWindowFlags kf =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBackground;

    auto draw_label = [&](float wx, float wy, float ox, float oy,
                          const char* text, bool anchor_right = false) {
        const Vec2 px = m_vp.world_to_pixel(wx, wy);
        if (px.x < -200.f || px.x > m_vp.dp_w+200.f) return;
        if (px.y < -200.f || px.y > m_vp.dp_h+200.f) return;
        m_api.push_math_font(true);
        const ImVec2 sz = ImGui::CalcTextSize(text);
        m_api.pop_math_font();
        const float fx = px.x + ox - (anchor_right ? sz.x : sz.x*0.5f);
        const float fy = px.y + oy - sz.y*0.5f;
        std::string wn = std::string("##lbl_") + text;
        ImGui::SetNextWindowPos(ImVec2(fx,fy), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.f);
        if (ImGui::Begin(wn.c_str(), nullptr, kf)) {
            m_api.push_math_font(true);
            ImGui::TextColored(label_col, "%s", text);
            m_api.pop_math_font();
        }
        ImGui::End();
    };

    // Greek letters used as sole content of a string literal are safe:
    // \xce\xb4 = δ (delta), \xce\xb5 = ε (epsilon) — each is the only
    // content in its string, so MSVC doesn't concatenate hex digits across
    // adjacent characters.
    draw_label(cx-eps, 0.f, 0.f, x_dy, "c-\xce\xb4");
    draw_label(cx,     0.f, 0.f, x_dy, "c");
    draw_label(cx+eps, 0.f, 0.f, x_dy, "c+\xce\xb4");
    const bool loa = snap_right_y;
    draw_label(0.f, cy-eps, y_dx, 0.f, "L(c)-\xce\xb5", loa);
    draw_label(0.f, cy,     y_dx, 0.f, "L(c)",          loa);
    draw_label(0.f, cy+eps, y_dx, 0.f, "L(c)+\xce\xb5", loa);
}

// ── submit_secant_line ────────────────────────────────────────────────────────

void Scene::submit_secant_line() {
    if (!m_hover.hit || !m_hover.has_secant || !m_analysis_panel.show_secant()) return;
    const float x0=m_vp.left(), y0=m_hover.slope*x0+m_hover.intercept;
    const float x1=m_vp.right(),y1=m_hover.slope*x1+m_hover.intercept;
    const float* rgb = m_analysis_panel.secant_colour();
    const Vec4 col{rgb[0],rgb[1],rgb[2],1.f};
    auto    slice = m_api.acquire(2);
    Vertex* v     = slice.vertices();
    v[0]={Vec3{x0,y0,0.f},col}; v[1]={Vec3{x1,y1,0.f},col};
    m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineList, DrawMode::VertexColor, col, m_vp.ortho_mvp());
}

// ── submit_tangent_line ───────────────────────────────────────────────────────

void Scene::submit_tangent_line() {
    if (!m_hover.hit || !m_hover.has_tangent || !m_analysis_panel.show_tangent()) return;
    const float m_    = m_hover.tangent_slope;
    const float b_int = m_hover.world_y - m_*m_hover.world_x;
    const float x0=m_vp.left(), y0=m_*x0+b_int;
    const float x1=m_vp.right(),y1=m_*x1+b_int;
    const float* rgb = m_analysis_panel.tangent_colour();
    const Vec4 col{rgb[0],rgb[1],rgb[2],1.f};
    auto    slice = m_api.acquire(2);
    Vertex* v     = slice.vertices();
    v[0]={Vec3{x0,y0,0.f},col}; v[1]={Vec3{x1,y1,0.f},col};
    m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineList, DrawMode::VertexColor, col, m_vp.ortho_mvp());
}

// ── submit_grid ───────────────────────────────────────────────────────────────

void Scene::submit_grid() {
    const Mat4 mvp = m_axes_cfg.is_3d ? m_vp.perspective_mvp() : m_vp.ortho_mvp();
    if (m_axes_cfg.is_3d) {
        math::AxesConfig cfg3d = m_axes_cfg;
        cfg3d.extent = m_axes_cfg.extent * 4.f;
        const u32 count = math::grid_vertex_count(cfg3d);
        if (!count) return;
        auto slice = m_api.acquire(count);
        math::build_grid({ slice.vertices(), count }, cfg3d);
        m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp);
    } else {
        const float vl=m_vp.left(),vr=m_vp.right(),vb=m_vp.bottom(),vt=m_vp.top();
        const u32 max_v = math::grid_vp_max_vertices(vl,vr,vb,vt,m_axes_cfg.minor_step);
        if (!max_v) return;
        auto slice = m_api.acquire(max_v);
        const u32 actual = math::build_grid_viewport(
            {slice.vertices(),max_v}, vl,vr,vb,vt,
            m_axes_cfg.minor_step, m_axes_cfg.major_step);
        if (!actual) return;
        memory::ArenaSlice trimmed = slice;
        trimmed.vertex_count = actual;
        m_api.submit_to(RenderTarget::Primary3D, trimmed, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp);
    }
}

// ── submit_axes ───────────────────────────────────────────────────────────────

void Scene::submit_axes() {
    const Mat4  mvp   = m_axes_cfg.is_3d ? m_vp.perspective_mvp() : m_vp.ortho_mvp();
    const float e     = m_axes_cfg.is_3d ? m_axes_cfg.extent*4.f : m_axes_cfg.extent;
    const u32   count = m_axes_cfg.is_3d ? 6u : 4u;
    auto    slice = m_api.acquire(count);
    Vertex* v     = slice.vertices();
    u32 i = 0;
    if (m_axes_cfg.is_3d) {
        v[i++]={Vec3{-e,0.f,0.f},math::colors::X_AXIS}; v[i++]={Vec3{e,0.f,0.f},math::colors::X_AXIS};
        v[i++]={Vec3{0.f,-e,0.f},math::colors::Y_AXIS}; v[i++]={Vec3{0.f,e,0.f},math::colors::Y_AXIS};
        v[i++]={Vec3{0.f,0.f,-e},math::colors::Z_AXIS}; v[i++]={Vec3{0.f,0.f,e},math::colors::Z_AXIS};
    } else {
        v[i++]={Vec3{m_vp.left(), 0.f,0.f},math::colors::X_AXIS};
        v[i++]={Vec3{m_vp.right(),0.f,0.f},math::colors::X_AXIS};
        v[i++]={Vec3{0.f,m_vp.bottom(),0.f},math::colors::Y_AXIS};
        v[i++]={Vec3{0.f,m_vp.top(),   0.f},math::colors::Y_AXIS};
    }
    m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp);
}

// ── submit_conics ─────────────────────────────────────────────────────────────

void Scene::submit_conics() {
    const Mat4 mvp = m_axes_cfg.is_3d ? m_vp.perspective_mvp() : m_vp.ortho_mvp();
    for (auto& entry : m_conics) {
        if (!entry.enabled) continue;
        if (entry.needs_rebuild) entry.rebuild();
        if (!entry.conic) continue;
        if ((entry.type == 2 || entry.type == 3) && !m_axes_cfg.is_3d) continue;
        if (entry.type == 1 && entry.hyp_two_branch) {
            auto* hyp   = static_cast<math::Hyperbola*>(entry.conic.get());
            const u32 n = hyp->two_branch_vertex_count(entry.tessellation);
            auto slice  = m_api.acquire(n);
            hyp->tessellate_two_branch({slice.vertices(),n}, entry.tessellation, entry.color);
            m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineStrip, DrawMode::VertexColor, entry.color, mvp);
        } else {
            const u32 n = entry.conic->vertex_count(entry.tessellation);
            auto slice  = m_api.acquire(n);
            entry.conic->tessellate({slice.vertices(),n}, entry.tessellation, entry.color);
            m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineStrip, DrawMode::VertexColor, entry.color, mvp);
        }
    }
}

// ── draw_main_panel ───────────────────────────────────────────────────────────

void Scene::draw_main_panel() {
    ImGui::SetNextWindowPos(ImVec2(20.f, 20.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.88f);
    ImGui::Begin("Scene");

    ImGui::SeparatorText("Grid");
    bool ext_changed = ImGui::SliderFloat("Extent",&m_axes_cfg.extent,0.5f,10.f,"%.1f");
    ImGui::SliderFloat("Major step",&m_axes_cfg.major_step,0.1f,2.f,"%.2f");
    ImGui::SliderFloat("Minor step",&m_axes_cfg.minor_step,0.05f,1.f,"%.2f");
    ImGui::Checkbox("3D mode", &m_axes_cfg.is_3d);
    if (ext_changed) m_vp.base_extent = m_axes_cfg.extent * 1.2f;

    ImGui::SeparatorText("View");
    if (!m_axes_cfg.is_3d)
        ImGui::Text("Pan (%.3f, %.3f)  Zoom %.2fx", m_vp.pan_x, m_vp.pan_y, m_vp.zoom);
    else
        ImGui::Text("Yaw %.2f  Pitch %.2f  Zoom %.2fx", m_vp.yaw, m_vp.pitch, m_vp.zoom);
    if (ImGui::Button("Reset View")) m_vp.reset();
    ImGui::TextDisabled(!m_axes_cfg.is_3d
        ? "Mid-drag/Alt+drag: pan  |  Scroll: zoom"
        : "Right/mid-drag: orbit   |  Scroll: zoom");

    ImGui::SeparatorText("Debug");
    if (ImGui::Button("Coord Debug")) m_coord_debug.visible() = !m_coord_debug.visible();
    ImGui::SameLine();
    if (m_perf_panel.visible()) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f,0.5f,0.2f,1.f));
        if (ImGui::Button("Perf Stats")) m_perf_panel.visible() = !m_perf_panel.visible();
        ImGui::PopStyleColor();
    } else {
        if (ImGui::Button("Perf Stats")) m_perf_panel.visible() = !m_perf_panel.visible();
    }
    {
        const auto& s = m_api.debug_stats();
        const ImVec4 fps_col = (s.fps >= 55.f) ? ImVec4(0.4f,1.f,0.4f,1.f)
            : (s.fps >= 30.f ? ImVec4(1.f,0.8f,0.f,1.f) : ImVec4(1.f,0.3f,0.3f,1.f));
        ImGui::SameLine();
        ImGui::TextColored(fps_col, "%.0f fps", s.fps);
        ImGui::TextDisabled("  Arena: %.1f%%  %llu verts  %u DC",
            s.arena_utilisation*100.f,
            static_cast<unsigned long long>(s.arena_vertex_count), s.draw_calls);
    }

    ImGui::Separator();
    ImGui::SeparatorText("Surfaces");
    for (int i = 0; i < static_cast<int>(m_surfaces.size()); ++i) {
        ImGui::PushID(100 + i);
        auto& e = m_surfaces[i];
        ImGui::Checkbox("##en", &e.enabled);
        ImGui::SameLine();
        if (ImGui::CollapsingHeader(e.name.c_str()))
            draw_surface_panel(e, i);
        ImGui::PopID();
    }
    if (ImGui::Button("+ Paraboloid", ImVec2(-1.f,0.f))) add_paraboloid();

    ImGui::Separator();
    ImGui::SeparatorText("Curves");
    for (int i = 0; i < static_cast<int>(m_conics.size()); ++i) {
        ImGui::PushID(i);
        auto& e = m_conics[i];
        ImGui::Checkbox("##en", &e.enabled);
        ImGui::SameLine();
        if (ImGui::CollapsingHeader(e.name.c_str()))
            draw_conic_panel(e, i);
        ImGui::PopID();
    }
    ImGui::Separator();
    if (ImGui::Button("+ Parabola",        ImVec2(-1.f,0.f))) add_parabola();
    if (ImGui::Button("+ Hyperbola",       ImVec2(-1.f,0.f))) add_hyperbola();
    if (ImGui::Button("+ Helix",           ImVec2(-1.f,0.f))) add_helix();
    if (ImGui::Button("+ Paraboloid Curve",ImVec2(-1.f,0.f))) add_paraboloid_curve();

    ImGui::End();
}

// ── draw_surface_panel ────────────────────────────────────────────────────────

void Scene::draw_surface_panel(SurfaceEntry& e, int /*idx*/) {
    ImGui::Indent();
    float col[4] = { e.color.r, e.color.g, e.color.b, e.color.a };
    if (ImGui::ColorEdit4("Color##s", col))
        e.color = Vec4{col[0],col[1],col[2],col[3]};

    ImGui::Separator();
    if (e.type == 0) {
        ImGui::TextDisabled("z = a(x^2 + y^2)   p(u,v) = (u*cos(v), u*sin(v), a*u^2)");

        bool d = false;
        d |= ImGui::SliderFloat("a##s",     &e.par_a,    0.1f, 4.f, "%.3f");
        d |= ImGui::SliderFloat("u max##s", &e.par_umax, 0.3f, 3.f, "%.2f");

        int ul = static_cast<int>(e.u_lines);
        int vl = static_cast<int>(e.v_lines);
        if (ImGui::SliderInt("u lines##s", &ul, 4, 32)) { e.u_lines = static_cast<u32>(ul); }
        if (ImGui::SliderInt("v lines##s", &vl, 4, 48)) { e.v_lines = static_cast<u32>(vl); }

        if (ImGui::Button("Reset##s")) {
            e.par_a = 1.f; e.par_umax = 1.5f;
            e.u_lines = 14; e.v_lines = 20; d = true;
        }
        if (d) e.mark_dirty();

        if (e.surface) {
            const auto* p = static_cast<math::Paraboloid*>(e.surface.get());
            ImGui::TextColored(ImVec4(0.7f,0.9f,1.f,1.f),
                "K(apex)=%.4f  K(u=%.1f)=%.4f",
                p->gaussian_curvature(0.f, 0.f),
                e.par_umax * 0.5f,
                p->gaussian_curvature(e.par_umax * 0.5f, 0.f));
            ImGui::TextColored(ImVec4(0.7f,0.9f,1.f,1.f),
                "H(apex)=%.4f  k1=%.4f  k2=%.4f",
                p->mean_curvature(0.f, 0.f),
                p->kappa1(0.f), p->kappa2(0.f));
        }
    }
    ImGui::Unindent();
}

// ── draw_conic_panel ──────────────────────────────────────────────────────────

void Scene::draw_conic_panel(ConicEntry& e, int /*idx*/) {
    ImGui::Indent();
    float col[4] = { e.color.r, e.color.g, e.color.b, e.color.a };
    if (ImGui::ColorEdit4("Color##c", col))
        e.color = Vec4{col[0],col[1],col[2],col[3]};
    int tess = static_cast<int>(e.tessellation);
    if (ImGui::SliderInt("Segments##c", &tess, 16, 1024))
        e.tessellation = static_cast<u32>(tess);
    ImGui::Separator();

    if (e.type == 0) {
        ImGui::Text("y = a*t^2 + b*t + c");
        bool d = false;
        d |= ImGui::SliderFloat("a##p",     &e.par_a,    -4.f, 4.f,  "%.3f");
        d |= ImGui::SliderFloat("b##p",     &e.par_b,    -4.f, 4.f,  "%.3f");
        d |= ImGui::SliderFloat("c##p",     &e.par_c,    -2.f, 2.f,  "%.3f");
        d |= ImGui::SliderFloat("t min##p", &e.par_tmin,-10.f, 0.f,  "%.2f");
        d |= ImGui::SliderFloat("t max##p", &e.par_tmax,  0.f,10.f,  "%.2f");
        if (ImGui::Button("Reset##p")) {
            e.par_a=-1.f; e.par_b=0.f; e.par_c=0.f;
            e.par_tmin=-5.f; e.par_tmax=5.f; d=true;
        }
        if (d) e.mark_dirty();
        if (e.conic) ImGui::TextDisabled("k(0)=%.5f  R=%.4f",
            e.conic->curvature(0.f),
            e.conic->curvature(0.f)>1e-8f ? 1.f/e.conic->curvature(0.f) : 0.f);

    } else if (e.type == 1) {
        ImGui::Text("(x-h)^2/a^2 - (y-k)^2/b^2 = 1");
        bool d = false;
        d |= ImGui::SliderFloat("a##h",     &e.hyp_a,    0.1f, 4.f,  "%.2f");
        d |= ImGui::SliderFloat("b##h",     &e.hyp_b,    0.1f, 4.f,  "%.2f");
        d |= ImGui::SliderFloat("h##h",     &e.hyp_h,   -2.f,  2.f,  "%.2f");
        d |= ImGui::SliderFloat("k##h",     &e.hyp_k,   -2.f,  2.f,  "%.2f");
        d |= ImGui::SliderFloat("range##h", &e.hyp_range,0.5f, 5.f,  "%.2f");
        const char* axis_names[] = { "Horizontal", "Vertical" };
        if (ImGui::Combo("Axis##h", &e.hyp_axis, axis_names, 2)) d = true;
        ImGui::Checkbox("Two branches##h", &e.hyp_two_branch);
        if (ImGui::Button("Reset##h")) {
            e.hyp_a=1.f; e.hyp_b=1.f; e.hyp_h=0.f; e.hyp_k=0.f; e.hyp_range=2.5f; d=true;
        }
        if (d) e.mark_dirty();

    } else if (e.type == 2) {
        ImGui::TextDisabled("p(t) = (r cos t, r sin t, b t),  b = pitch/2pi");
        bool d = false;
        d |= ImGui::SliderFloat("radius##h", &e.hel_radius, 0.1f, 3.f, "%.3f");
        d |= ImGui::SliderFloat("pitch##h",  &e.hel_pitch,  0.05f,2.f, "%.3f");
        d |= ImGui::SliderFloat("t min##h",  &e.hel_tmin, -20.f, 0.f, "%.2f");
        d |= ImGui::SliderFloat("t max##h",  &e.hel_tmax,   0.f,20.f, "%.2f");
        if (ImGui::Button("Reset##helx")) {
            e.hel_radius=1.f; e.hel_pitch=0.5f;
            e.hel_tmin=0.f; e.hel_tmax=4.f*std::numbers::pi_v<float>; d=true;
        }
        if (d) e.mark_dirty();
        if (e.conic) {
            const float kappa = e.conic->curvature(0.f);
            const float tau   = e.conic->torsion(0.f);
            ImGui::TextColored(ImVec4(0.8f,0.9f,1.f,1.f),
                "k=%.5f  R=%.4f  t=%.5f", kappa,
                kappa>1e-8f ? 1.f/kappa : 0.f, tau);
        }

    } else if (e.type == 3) {
        ImGui::TextDisabled("p(t) = (t*cos(theta),  t*sin(theta),  a*t^2)");
        ImGui::TextDisabled("Meridional parabola on  z = a(x^2 + y^2)");
        bool d = false;
        d |= ImGui::SliderFloat("a##pc",     &e.pc_a,      0.1f, 4.f, "%.3f");
        float deg = e.pc_theta * (180.f / std::numbers::pi_v<float>);
        if (ImGui::SliderFloat("theta (deg)##pc", &deg, -180.f, 180.f, "%.1f")) {
            e.pc_theta = deg * (std::numbers::pi_v<float> / 180.f);
            d = true;
        }
        d |= ImGui::SliderFloat("t min##pc", &e.pc_tmin, 0.f,  2.f, "%.3f");
        d |= ImGui::SliderFloat("t max##pc", &e.pc_tmax, 0.1f, 3.f, "%.3f");
        if (ImGui::Button("Reset##pc")) {
            e.pc_a=1.f; e.pc_theta=0.f; e.pc_tmin=0.f; e.pc_tmax=1.5f; d=true;
        }
        if (d) e.mark_dirty();
        if (e.conic) {
            const float tmid  = (e.pc_tmin + e.pc_tmax) * 0.5f;
            const float kappa = e.conic->curvature(tmid);
            ImGui::TextColored(ImVec4(1.f,0.7f,0.3f,1.f),
                "k(t=%.2f)=%.5f  R=%.4f  tau=%.3f",
                tmid, kappa, kappa>1e-8f ? 1.f/kappa : 0.f, e.conic->torsion(tmid));
            ImGui::TextDisabled("k = kappa1 of paraboloid  |  torsion = 0 (planar)");
        }
    }
    ImGui::Unindent();
}

} // namespace ndde
