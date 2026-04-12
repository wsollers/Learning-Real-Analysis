// app/Scene.cpp
#include "app/Scene.hpp"
#include <imgui.h>
#include <format>
#include <cmath>
#include <numbers>
#include <limits>
#include <algorithm>

namespace ndde {

// ═══════════════════════════════════════════════════════════════════════════════
// ConicEntry
// ═══════════════════════════════════════════════════════════════════════════════

void ConicEntry::rebuild() {
    needs_rebuild = false;
    if (type == 0) {
        conic = std::make_unique<math::Parabola>(par_a, par_b, par_c, par_tmin, par_tmax);
    } else {
        const auto axis = (hyp_axis == 0)
            ? math::HyperbolaAxis::Horizontal
            : math::HyperbolaAxis::Vertical;
        conic = std::make_unique<math::Hyperbola>(hyp_a, hyp_b, hyp_h, hyp_k, axis, hyp_range);
    }

    snap_cache.clear();
    if (!conic) return;

    if (type == 1 && hyp_two_branch) {
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
// Scene
// ═══════════════════════════════════════════════════════════════════════════════

Scene::Scene(EngineAPI api) : m_api(std::move(api)) {
    m_vp.base_extent = m_axes_cfg.extent * 1.2f;
    add_parabola();
    add_hyperbola();
}

void Scene::add_parabola() {
    ConicEntry e;
    e.name         = std::format("Parabola {}", m_conics.size());
    e.type         = 0;
    e.color        = { 1.0f, 0.8f, 0.2f, 1.0f };
    e.tessellation = m_api.config().simulation.tessellation;
    e.par_a    = -1.f;
    e.par_b    =  0.f;
    e.par_c    =  0.f;
    e.par_tmin = -5.f;
    e.par_tmax =  5.f;
    e.rebuild();
    m_conics.push_back(std::move(e));
}

void Scene::add_hyperbola() {
    ConicEntry e;
    e.name         = std::format("Hyperbola {}", m_conics.size());
    e.type         = 1;
    e.color        = { 0.4f, 0.8f, 1.0f, 1.0f };
    e.tessellation = m_api.config().simulation.tessellation;
    e.hyp_range = 2.5f;
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

    update_hover();
    m_coord_debug.update(m_vp, m_hover, m_conics, m_api.viewport_size());

    draw_main_panel();
    m_analysis_panel.draw(m_hover, m_api.math_font_body());
    m_coord_debug.draw();
    submit_interval_labels();   // ImGui overlay — must be in ImGui render pass

    submit_grid();
    submit_axes();
    submit_conics();
    submit_epsilon_ball();
    submit_interval_lines();
    submit_secant_line();
    submit_tangent_line();
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

    if (std::abs(io.MouseWheel) > 0.f)
        m_vp.zoom_toward(io.MousePos.x, io.MousePos.y, io.MouseWheel);

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

// ── update_hover ─────────────────────────────────────────────────────────────

void Scene::update_hover() {
    m_hover = HoverResult{};
    if (m_axes_cfg.is_3d) return;

    const ImGuiIO& io = ImGui::GetIO();
    if (io.MousePos.x < 0.f || io.MousePos.y < 0.f) return;
    if (ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered()) return;

    const Vec2  mw = m_vp.pixel_to_world(io.MousePos.x, io.MousePos.y);

    const float world_per_px = (m_vp.dp_h > 0.f)
        ? (m_vp.top() - m_vp.bottom()) / m_vp.dp_h
        : 0.01f;
    const float snap_r_world = m_analysis_panel.get_snap_px_radius() * world_per_px;

    float best_d  = snap_r_world;
    int   best_ci = -1, best_si = -1;
    float best_x  = 0.f, best_y = 0.f;

    for (int ci = 0; ci < static_cast<int>(m_conics.size()); ++ci) {
        const auto& entry = m_conics[ci];
        if (!entry.enabled || entry.snap_cache.empty()) continue;
        for (int si = 0; si < static_cast<int>(entry.snap_cache.size()); ++si) {
            const auto [px, py] = entry.snap_cache[si];
            const float d = std::hypot(px - mw.x, py - mw.y);
            if (d < best_d) {
                best_d = d; best_ci = ci; best_si = si;
                best_x = px; best_y = py;
            }
        }
    }

    if (best_ci < 0) return;

    m_hover.hit       = true;
    m_hover.world_x   = best_x;
    m_hover.world_y   = best_y;
    m_hover.curve_idx = best_ci;
    m_hover.snap_idx  = best_si;

    const float secant_r  = m_analysis_panel.get_epsilon_ball_radius();
    const auto& cache     = m_conics[best_ci].snap_cache;
    const int   total     = static_cast<int>(cache.size());
    const auto& e_ref     = m_conics[best_ci];

    const int branch_end = (e_ref.type == 1 && e_ref.hyp_two_branch)
        ? (best_si <= static_cast<int>(e_ref.tessellation)
            ? static_cast<int>(e_ref.tessellation)
            : total - 1)
        : total - 1;
    const int branch_start = (e_ref.type == 1 && e_ref.hyp_two_branch)
        ? (best_si <= static_cast<int>(e_ref.tessellation)
            ? 0
            : static_cast<int>(e_ref.tessellation) + 1)
        : 0;

    int li = best_si;
    while (li > branch_start && std::hypot(cache[li-1].first  - best_x,
                                            cache[li-1].second - best_y) <= secant_r) --li;
    int ri = best_si;
    while (ri < branch_end && std::hypot(cache[ri+1].first  - best_x,
                                          cache[ri+1].second - best_y) <= secant_r) ++ri;

    if (li != ri) {
        const auto [x0, y0] = cache[li];
        const auto [x1, y1] = cache[ri];
        const float dx = x1 - x0, dy = y1 - y0;
        m_hover.has_secant  = true;
        m_hover.secant_x0   = x0; m_hover.secant_y0 = y0;
        m_hover.secant_x1   = x1; m_hover.secant_y1 = y1;
        m_hover.slope       = (std::abs(dx) > 1e-9f) ? dy / dx : 0.f;
        m_hover.intercept   = y0 - m_hover.slope * x0;
    }

    if (e_ref.conic) {
        const auto& c        = *e_ref.conic;
        const int   branch_i = best_si - branch_start;
        const int   branch_n = branch_end - branch_start;
        const float frac     = (branch_n > 0)
            ? static_cast<float>(branch_i) / static_cast<float>(branch_n)
            : 0.f;
        const float t = c.t_min() + frac * (c.t_max() - c.t_min());
        const Vec3  d = c.derivative(t);
        if (!(e_ref.type == 1 && e_ref.hyp_two_branch &&
              best_si > static_cast<int>(e_ref.tessellation))) {
            m_hover.has_tangent   = true;
            m_hover.tangent_slope = (std::abs(d.x) > 1e-9f) ? d.y / d.x : 0.f;
        } else if (e_ref.hyp_axis == 0) {
            const float neg_dx = -d.x;
            m_hover.has_tangent   = true;
            m_hover.tangent_slope = (std::abs(neg_dx) > 1e-9f) ? d.y / neg_dx : 0.f;
        } else {
            const float neg_dy = -d.y;
            m_hover.has_tangent   = true;
            m_hover.tangent_slope = (std::abs(d.x) > 1e-9f) ? neg_dy / d.x : 0.f;
        }
    }
}

// ── submit_epsilon_ball ───────────────────────────────────────────────────────

void Scene::submit_epsilon_ball() {
    if (!m_hover.hit)                          return;
    if (!m_analysis_panel.show_epsilon_ball()) return;

    constexpr u32 segments = 64;
    constexpr u32 count    = segments + 1;

    const float r  = m_analysis_panel.get_epsilon_ball_radius();
    const float cx = m_hover.world_x;
    const float cy = m_hover.world_y;
    const Vec4  col = { 0.95f, 0.95f, 0.15f, 0.9f };

    auto    slice = m_api.acquire(count);
    Vertex* v     = slice.vertices();
    for (u32 i = 0; i < segments; ++i) {
        const float theta = (static_cast<float>(i) / static_cast<float>(segments))
                            * 2.f * std::numbers::pi_v<float>;
        v[i] = { Vec3{ cx + r * std::cos(theta), cy + r * std::sin(theta), 0.f }, col };
    }
    v[segments] = v[0];
    m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor, col, m_vp.ortho_mvp());
}

// ── submit_interval_lines ─────────────────────────────────────────────────────

void Scene::submit_interval_lines() {
    if (!m_hover.hit)                             return;
    if (!m_analysis_panel.show_interval_lines())  return;

    const float cx  = m_hover.world_x;
    const float cy  = m_hover.world_y;
    const float eps = m_analysis_panel.get_epsilon_interval();

    const float* rgb = m_analysis_panel.interval_colour();
    const Vec4   col = { rgb[0], rgb[1], rgb[2], 0.8f };
    const Vec4   ctr = { rgb[0]*0.6f, rgb[1]*0.6f, rgb[2]*0.6f, 0.55f };

    auto    slice = m_api.acquire(24);
    Vertex* v     = slice.vertices();
    u32     idx   = 0;

    auto push = [&](float x0, float y0, float x1, float y1, Vec4 c) {
        v[idx++] = { Vec3{x0, y0, 0.f}, c };
        v[idx++] = { Vec3{x1, y1, 0.f}, c };
    };

    push(cx-eps, cy-eps, cx+eps, cy-eps, col);
    push(cx-eps, cy+eps, cx+eps, cy+eps, col);
    push(cx-eps, cy-eps, cx-eps, cy+eps, col);
    push(cx+eps, cy-eps, cx+eps, cy+eps, col);
    push(cx,  0.f, cx,  cy,  ctr);
    push(0.f, cy,  cx,  cy,  ctr);
    const float tk = eps * 0.4f;
    push(cx-tk, 0.f, cx+tk, 0.f, col);
    push(0.f, cy-tk, 0.f, cy+tk, col);
    push(cx-eps, 0.f, cx-eps, tk*2.f, col);
    push(cx+eps, 0.f, cx+eps, tk*2.f, col);
    push(0.f, cy-eps, tk*2.f, cy-eps, col);
    push(0.f, cy+eps, tk*2.f, cy+eps, col);

    m_api.submit(slice, Topology::LineList, DrawMode::VertexColor, col, m_vp.ortho_mvp());
}

// ── submit_interval_labels ────────────────────────────────────────────────────
// Renders six ImGui overlay labels at the axis intercept points:
//
//   x-axis:   c-δ        c       c+δ
//   y-axis:  L(c)-ε    L(c)    L(c)+ε
//
// Each label is a minimal transparent ImGui window positioned via
// world_to_pixel(), nudged away from the tick marks.

void Scene::submit_interval_labels() {
    if (!m_hover.hit)                             return;
    if (!m_analysis_panel.show_interval_lines())  return;

    const float cx  = m_hover.world_x;
    const float cy  = m_hover.world_y;
    const float eps = m_analysis_panel.get_epsilon_interval();

    const float* rgb = m_analysis_panel.interval_colour();
    const ImVec4 label_col{ rgb[0], rgb[1], rgb[2], 0.95f };

    constexpr float PAD = 6.f;

    const bool snap_above_x = (cy > 0.f);
    const bool snap_right_y = (cx > 0.f);

    const float x_label_dy = snap_above_x ?  PAD : -PAD;
    const float y_label_dx = snap_right_y ? -PAD :  PAD;

    constexpr ImGuiWindowFlags k_label_flags =
        ImGuiWindowFlags_NoTitleBar       |
        ImGuiWindowFlags_NoResize         |
        ImGuiWindowFlags_NoScrollbar      |
        ImGuiWindowFlags_NoInputs         |
        ImGuiWindowFlags_NoSavedSettings  |
        ImGuiWindowFlags_NoNav            |
        ImGuiWindowFlags_NoMove           |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBackground;

    ImFont* font = m_api.math_font_small();

    // draw_label: places a transparent ImGui window with a single text label.
    // anchor_right: if true, the label's right edge is at (px + offset),
    //               used for y-axis labels to clear the axis line on the left.
    //
    // NOTE: ImFont::FontSize was removed in newer ImGui (this codebase uses
    // a restructured version).  Text sizing uses ImGui::CalcTextSize() with
    // the font pushed, which queries the current scaled size correctly.
    auto draw_label = [&](float world_x, float world_y,
                          float px_offset_x, float px_offset_y,
                          const char* text, bool anchor_right = false)
    {
        const Vec2 px = m_vp.world_to_pixel(world_x, world_y);
        if (px.x < -200.f || px.x > m_vp.dp_w + 200.f) return;
        if (px.y < -200.f || px.y > m_vp.dp_h + 200.f) return;

        // Measure text with the correct font pushed so GetTextLineHeight()
        // reflects the small font size.  We must push before CalcTextSize
        // because this ImGui version ties size to the pushed font state.
        float text_w = 0.f;
        float text_h = 0.f;
        if (font) ImGui::PushFont(font);
        {
            const ImVec2 sz = ImGui::CalcTextSize(text);
            text_w = sz.x;
            text_h = sz.y;
        }
        if (font) ImGui::PopFont();

        const float wx = px.x + px_offset_x - (anchor_right ? text_w : 0.f);
        const float wy = px.y + px_offset_y - text_h * 0.5f;

        std::string win_name = std::string("##lbl_") + text;
        ImGui::SetNextWindowPos(ImVec2(wx, wy), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.f);
        if (ImGui::Begin(win_name.c_str(), nullptr, k_label_flags)) {
            if (font) ImGui::PushFont(font);
            ImGui::TextColored(label_col, "%s", text);
            if (font) ImGui::PopFont();
        }
        ImGui::End();
    };

    // x-axis: c-δ, c, c+δ
    // UTF-8: δ = U+03B4 = 0xCE 0xB4 | ε = U+03B5 = 0xCE 0xB5
    draw_label(cx - eps, 0.f, -4.f, x_label_dy, "c-\xce\xb4");
    draw_label(cx,       0.f,  0.f, x_label_dy, "c");
    draw_label(cx + eps, 0.f,  4.f, x_label_dy, "c+\xce\xb4");

    // y-axis: L(c)-ε, L(c), L(c)+ε
    const bool left_of_axis = snap_right_y;
    draw_label(0.f, cy - eps, y_label_dx, -8.f, "L(c)-\xce\xb5", left_of_axis);
    draw_label(0.f, cy,       y_label_dx, -8.f, "L(c)",          left_of_axis);
    draw_label(0.f, cy + eps, y_label_dx, -8.f, "L(c)+\xce\xb5", left_of_axis);
}

// ── submit_secant_line ────────────────────────────────────────────────────────

void Scene::submit_secant_line() {
    if (!m_hover.hit || !m_hover.has_secant) return;
    if (!m_analysis_panel.show_secant())      return;

    const float x0 = m_vp.left(),  y0 = m_hover.slope * x0 + m_hover.intercept;
    const float x1 = m_vp.right(), y1 = m_hover.slope * x1 + m_hover.intercept;

    const float* rgb = m_analysis_panel.secant_colour();
    const Vec4   col = { rgb[0], rgb[1], rgb[2], 1.f };

    auto    slice = m_api.acquire(2);
    Vertex* v     = slice.vertices();
    v[0] = { Vec3{x0, y0, 0.f}, col };
    v[1] = { Vec3{x1, y1, 0.f}, col };
    m_api.submit(slice, Topology::LineList, DrawMode::VertexColor, col, m_vp.ortho_mvp());
}

// ── submit_tangent_line ───────────────────────────────────────────────────────

void Scene::submit_tangent_line() {
    if (!m_hover.hit || !m_hover.has_tangent) return;
    if (!m_analysis_panel.show_tangent())      return;

    const float m     = m_hover.tangent_slope;
    const float b_int = m_hover.world_y - m * m_hover.world_x;
    const float x0    = m_vp.left(),  y0 = m * x0 + b_int;
    const float x1    = m_vp.right(), y1 = m * x1 + b_int;

    const float* rgb = m_analysis_panel.tangent_colour();
    const Vec4   col = { rgb[0], rgb[1], rgb[2], 1.f };

    auto    slice = m_api.acquire(2);
    Vertex* v     = slice.vertices();
    v[0] = { Vec3{x0, y0, 0.f}, col };
    v[1] = { Vec3{x1, y1, 0.f}, col };
    m_api.submit(slice, Topology::LineList, DrawMode::VertexColor, col, m_vp.ortho_mvp());
}

// ── submit_grid ───────────────────────────────────────────────────────────────

void Scene::submit_grid() {
    const Mat4 mvp = m_axes_cfg.is_3d ? m_vp.perspective_mvp() : m_vp.ortho_mvp();

    if (m_axes_cfg.is_3d) {
        math::AxesConfig cfg3d = m_axes_cfg;
        cfg3d.extent = m_axes_cfg.extent * 4.f;
        const u32 count = math::grid_vertex_count(cfg3d);
        if (count == 0) return;
        auto slice = m_api.acquire(count);
        math::build_grid({ slice.vertices(), count }, cfg3d);
        m_api.submit(slice, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp);
    } else {
        const float vl = m_vp.left(), vr = m_vp.right();
        const float vb = m_vp.bottom(), vt = m_vp.top();
        const u32 max_v = math::grid_vp_max_vertices(vl, vr, vb, vt, m_axes_cfg.minor_step);
        if (max_v == 0) return;
        auto slice = m_api.acquire(max_v);
        const u32 actual = math::build_grid_viewport(
            { slice.vertices(), max_v }, vl, vr, vb, vt,
            m_axes_cfg.minor_step, m_axes_cfg.major_step);
        if (actual == 0) return;
        memory::ArenaSlice trimmed = slice;
        trimmed.vertex_count = actual;
        m_api.submit(trimmed, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp);
    }
}

// ── submit_axes ───────────────────────────────────────────────────────────────

void Scene::submit_axes() {
    const Mat4  mvp   = m_axes_cfg.is_3d ? m_vp.perspective_mvp() : m_vp.ortho_mvp();
    const float e     = m_axes_cfg.is_3d ? m_axes_cfg.extent * 4.f : m_axes_cfg.extent;
    const u32   count = m_axes_cfg.is_3d ? 6u : 4u;

    auto    slice = m_api.acquire(count);
    Vertex* v     = slice.vertices();
    u32     i     = 0;

    if (m_axes_cfg.is_3d) {
        v[i++] = { Vec3{-e,  0.f, 0.f}, math::colors::X_AXIS };
        v[i++] = { Vec3{ e,  0.f, 0.f}, math::colors::X_AXIS };
        v[i++] = { Vec3{0.f, -e,  0.f}, math::colors::Y_AXIS };
        v[i++] = { Vec3{0.f,  e,  0.f}, math::colors::Y_AXIS };
        v[i++] = { Vec3{0.f, 0.f, -e},  math::colors::Z_AXIS };
        v[i++] = { Vec3{0.f, 0.f,  e},  math::colors::Z_AXIS };
    } else {
        v[i++] = { Vec3{m_vp.left(),   0.f,           0.f}, math::colors::X_AXIS };
        v[i++] = { Vec3{m_vp.right(),  0.f,           0.f}, math::colors::X_AXIS };
        v[i++] = { Vec3{0.f,           m_vp.bottom(), 0.f}, math::colors::Y_AXIS };
        v[i++] = { Vec3{0.f,           m_vp.top(),    0.f}, math::colors::Y_AXIS };
    }

    m_api.submit(slice, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp);
}

// ── submit_conics ─────────────────────────────────────────────────────────────

void Scene::submit_conics() {
    const Mat4 mvp = m_axes_cfg.is_3d ? m_vp.perspective_mvp() : m_vp.ortho_mvp();

    for (auto& entry : m_conics) {
        if (!entry.enabled) continue;
        if (entry.needs_rebuild) entry.rebuild();
        if (!entry.conic) continue;

        if (entry.type == 1 && entry.hyp_two_branch) {
            auto* hyp   = static_cast<math::Hyperbola*>(entry.conic.get());
            const u32 n = hyp->two_branch_vertex_count(entry.tessellation);
            auto slice  = m_api.acquire(n);
            hyp->tessellate_two_branch({ slice.vertices(), n }, entry.tessellation, entry.color);
            m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor, entry.color, mvp);
        } else {
            const u32 n = entry.conic->vertex_count(entry.tessellation);
            auto slice  = m_api.acquire(n);
            entry.conic->tessellate({ slice.vertices(), n }, entry.tessellation, entry.color);
            m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor, entry.color, mvp);
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
    bool ext_changed = ImGui::SliderFloat("Extent",     &m_axes_cfg.extent,     0.5f, 10.f, "%.1f");
    ImGui::SliderFloat("Major step", &m_axes_cfg.major_step, 0.1f, 2.f,  "%.2f");
    ImGui::SliderFloat("Minor step", &m_axes_cfg.minor_step, 0.05f, 1.f, "%.2f");
    ImGui::Checkbox("3D mode", &m_axes_cfg.is_3d);
    if (ext_changed) m_vp.base_extent = m_axes_cfg.extent * 1.2f;

    ImGui::SeparatorText("View");
    if (!m_axes_cfg.is_3d)
        ImGui::Text("Pan (%.3f, %.3f)  Zoom %.2fx", m_vp.pan_x, m_vp.pan_y, m_vp.zoom);
    else
        ImGui::Text("Yaw %.2f  Pitch %.2f  Zoom %.2fx", m_vp.yaw, m_vp.pitch, m_vp.zoom);
    if (ImGui::Button("Reset View")) m_vp.reset();
    if (!m_axes_cfg.is_3d) {
        ImGui::TextDisabled("Middle-drag or Alt+drag: pan");
        ImGui::TextDisabled("Scroll: zoom toward cursor");
    } else {
        ImGui::TextDisabled("Right-drag or middle-drag: orbit");
        ImGui::TextDisabled("Scroll: zoom");
    }

    ImGui::SeparatorText("Debug");
    if (ImGui::Button("Coord Debug Window"))
        m_coord_debug.visible() = !m_coord_debug.visible();

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
    if (ImGui::Button("+ Parabola",  ImVec2(-1.f, 0.f))) add_parabola();
    if (ImGui::Button("+ Hyperbola", ImVec2(-1.f, 0.f))) add_hyperbola();

    ImGui::End();
}

// ── draw_conic_panel ──────────────────────────────────────────────────────────

void Scene::draw_conic_panel(ConicEntry& e, int /*idx*/) {
    ImGui::Indent();

    float col[4] = { e.color.r, e.color.g, e.color.b, e.color.a };
    if (ImGui::ColorEdit4("Color##c", col))
        e.color = Vec4{ col[0], col[1], col[2], col[3] };

    int tess = static_cast<int>(e.tessellation);
    if (ImGui::SliderInt("Segments##c", &tess, 16, 1024))
        e.tessellation = static_cast<u32>(tess);

    ImGui::Separator();

    if (e.type == 0) {
        ImGui::Text("y = a*t^2 + b*t + c");
        bool d = false;
        d |= ImGui::SliderFloat("a##p",     &e.par_a,    -4.f, 4.f, "%.3f");
        d |= ImGui::SliderFloat("b##p",     &e.par_b,    -4.f, 4.f, "%.3f");
        d |= ImGui::SliderFloat("c##p",     &e.par_c,    -2.f, 2.f, "%.3f");
        d |= ImGui::SliderFloat("t min##p", &e.par_tmin, -10.f, 0.f, "%.2f");
        d |= ImGui::SliderFloat("t max##p", &e.par_tmax,   0.f, 10.f, "%.2f");
        if (ImGui::Button("Reset##p")) {
            e.par_a = -1.f; e.par_b = 0.f; e.par_c = 0.f;
            e.par_tmin = -5.f; e.par_tmax = 5.f; d = true;
        }
        if (d) e.mark_dirty();
        if (e.conic) ImGui::TextDisabled("k(0) = %.5f", e.conic->curvature(0.f));
    } else {
        ImGui::Text("(x-h)^2/a^2 - (y-k)^2/b^2 = 1");
        bool d = false;
        d |= ImGui::SliderFloat("a##h",     &e.hyp_a,     0.1f, 4.f, "%.2f");
        d |= ImGui::SliderFloat("b##h",     &e.hyp_b,     0.1f, 4.f, "%.2f");
        d |= ImGui::SliderFloat("h##h",     &e.hyp_h,    -2.f,  2.f, "%.2f");
        d |= ImGui::SliderFloat("k##h",     &e.hyp_k,    -2.f,  2.f, "%.2f");
        d |= ImGui::SliderFloat("range##h", &e.hyp_range, 0.5f, 5.f, "%.2f");
        const char* axis_names[] = { "Horizontal", "Vertical" };
        if (ImGui::Combo("Axis##h", &e.hyp_axis, axis_names, 2)) d = true;
        ImGui::Checkbox("Two branches##h", &e.hyp_two_branch);
        if (ImGui::Button("Reset##h")) {
            e.hyp_a = 1.f; e.hyp_b = 1.f; e.hyp_h = 0.f;
            e.hyp_k = 0.f; e.hyp_range = 2.5f; d = true;
        }
        if (d) e.mark_dirty();
    }

    ImGui::Unindent();
}

} // namespace ndde
