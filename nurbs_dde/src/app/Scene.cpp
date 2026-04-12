// app/Scene.cpp
#include "app/Scene.hpp"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <format>
#include <stdexcept>
#include <cmath>
#include <numbers>
#include <limits>

namespace ndde {

// ── ConicEntry::rebuild ───────────────────────────────────────────────────────

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
    // Rebuild the CPU-side vertex cache used for hover snap
    snap_cache.clear();
    if (conic) {
        const u32 n = tessellation;
        snap_cache.resize(n + 1u);
        const float t0   = conic->t_min();
        const float step = (conic->t_max() - t0) / static_cast<float>(n);
        for (u32 i = 0; i <= n; ++i) {
            const float t = t0 + static_cast<float>(i) * step;
            const auto p = conic->evaluate(t);
            snap_cache[i] = { p.x, p.y };
        }
    }
}

// ── Scene ctor ────────────────────────────────────────────────────────────────

Scene::Scene(EngineAPI api) : m_api(std::move(api)) {
    add_parabola();
    add_hyperbola();
}

void Scene::add_parabola() {
    ConicEntry e;
    e.name         = std::format("Parabola {}", m_conics.size());
    e.type         = 0;
    e.color        = { 1.0f, 0.8f, 0.2f, 1.0f };
    e.tessellation = m_api.config().simulation.tessellation;
    e.rebuild();
    m_conics.push_back(std::move(e));
}

void Scene::add_hyperbola() {
    ConicEntry e;
    e.name         = std::format("Hyperbola {}", m_conics.size());
    e.type         = 1;
    e.color        = { 0.4f, 0.8f, 1.0f, 1.0f };
    e.tessellation = m_api.config().simulation.tessellation;
    e.rebuild();
    m_conics.push_back(std::move(e));
}

// ── on_frame ──────────────────────────────────────────────────────────────────

void Scene::on_frame() {
    update_hover();     // 1. mouse → world, snap to nearest curve point

    draw_main_panel();
    m_analysis_panel.draw(m_hover, m_api.math_font_body());

    submit_grid();
    submit_axes();
    submit_conics();
    submit_epsilon_ball(); // 2. draw the ball if snap hit and toggle is on
}

// ── ortho_mvp ─────────────────────────────────────────────────────────────────

Mat4 Scene::ortho_mvp() const noexcept {
    /*
    const float e = m_axes_cfg.extent * 1.2f;
    return glm::ortho(-e, e, -e, e, -10.f, 10.f);
    */
    const float e = m_axes_cfg.extent * 1.2f;

    // Get actual window dimensions from the API/Config
    float width  = static_cast<float>(m_api.config().window.width);
    float height = static_cast<float>(m_api.config().window.height);
    float aspect = width / height;

    // OLD: return glm::ortho(-e, e, -e, e, -10.f, 10.f);
    // NEW: Scale the X bounds by the aspect ratio
    return glm::ortho(-e * aspect, e * aspect, -e, e, -10.f, 10.f);
}

// ── update_hover ──────────────────────────────────────────────────────────────
// Converts the ImGui mouse position to world space, then snaps to the nearest
// tessellated curve point within the epsilon ball radius.

void Scene::update_hover() {
    m_hover = HoverResult{};  // reset each frame

    // Only hit-test when the mouse is over the viewport, not over an ImGui window
    if (ImGui::GetIO().WantCaptureMouse) return;

    const ImVec2 mouse_px = ImGui::GetMousePos();
    const ImVec2 display  = ImGui::GetIO().DisplaySize;
    if (display.x <= 0.f || display.y <= 0.f) return;

    // Normalised Device Coordinates: [-1, 1]
    const float ndc_x = (mouse_px.x / display.x) * 2.f - 1.f;
    const float ndc_y = 1.f - (mouse_px.y / display.y) * 2.f; // Y flipped

    // Invert the orthographic MVP to get world-space mouse position.
    // glm::ortho produces a matrix whose inverse maps NDC -> world.
    const Mat4 mvp     = ortho_mvp();
    const Mat4 inv_mvp = glm::inverse(mvp);
    const Vec4 world4  = inv_mvp * Vec4{ ndc_x, ndc_y, 0.f, 1.f };
    const float mx = world4.x;
    const float my = world4.y;

    const float snap_radius = m_analysis_panel.get_epsilon_ball_radius();
    const float snap_r2     = snap_radius * snap_radius;

    float   best_dist2  = std::numeric_limits<float>::max();
    int     best_curve  = -1;
    int     best_idx    = -1;
    float   best_x      = 0.f;
    float   best_y      = 0.f;

    for (int ci = 0; ci < static_cast<int>(m_conics.size()); ++ci) {
        const auto& entry = m_conics[ci];
        if (!entry.enabled || entry.snap_cache.empty()) continue;

        for (int si = 0; si < static_cast<int>(entry.snap_cache.size()); ++si) {
            const auto& [px, py] = entry.snap_cache[si];
            const float dx = px - mx;
            const float dy = py - my;
            const float d2 = dx * dx + dy * dy;
            if (d2 < best_dist2) {
                best_dist2 = d2;
                best_curve = ci;
                best_idx   = si;
                best_x     = px;
                best_y     = py;
            }
        }
    }

    if (best_curve >= 0 && best_dist2 <= snap_r2) {
        m_hover.hit       = true;
        m_hover.world_x   = best_x;
        m_hover.world_y   = best_y;
        m_hover.curve_idx = best_curve;
        m_hover.snap_idx  = best_idx;

        // Secant: use the adjacent tessellation point
        const auto& cache = m_conics[best_curve].snap_cache;
        const int n = static_cast<int>(cache.size());
        if (best_idx + 1 < n) {
            m_hover.has_secant  = true;
            m_hover.secant_x0   = best_x;
            m_hover.secant_y0   = best_y;
            m_hover.secant_x1   = cache[best_idx + 1].first;
            m_hover.secant_y1   = cache[best_idx + 1].second;
            const float dx = m_hover.secant_x1 - m_hover.secant_x0;
            const float dy = m_hover.secant_y1 - m_hover.secant_y0;
            m_hover.slope     = (std::abs(dx) > 1e-8f) ? dy / dx : 0.f;
            m_hover.intercept = m_hover.secant_y0 - m_hover.slope * m_hover.secant_x0;
        }

        // Tangent: use the analytic derivative from the conic
        if (m_conics[best_curve].conic) {
            const auto& conic = *m_conics[best_curve].conic;
            const float t0    = conic.t_min();
            const float tspan = conic.t_max() - t0;
            const float t     = t0 + (static_cast<float>(best_idx) /
                                      static_cast<float>(cache.size() - 1)) * tspan;
            const Vec3 d = conic.derivative(t);
            m_hover.has_tangent   = true;
            m_hover.tangent_slope = (std::abs(d.x) > 1e-8f) ? d.y / d.x : 0.f;
        }
    }
}

// ── submit_epsilon_ball ───────────────────────────────────────────────────────
// Tessellates a circle of radius epsilon around the snapped point.
// Uses LineStrip with the last vertex equal to the first to close the loop.

void Scene::submit_epsilon_ball() {
    if (!m_hover.hit)                           return;
    if (!m_analysis_panel.show_epsilon_ball())  return;

    constexpr u32 segments = 64;
    constexpr u32 count    = segments + 1;  // +1 to close the loop

    const float r   = m_analysis_panel.get_epsilon_ball_radius();
    const float cx  = m_hover.world_x;
    const float cy  = m_hover.world_y;
    const Vec4  col = { 0.9f, 0.9f, 0.2f, 0.85f }; // bright yellow

    auto slice = m_api.acquire(count);
    Vertex* v  = slice.vertices();

    for (u32 i = 0; i < segments; ++i) {
        const float theta = (static_cast<float>(i) / static_cast<float>(segments))
                            * 2.f * std::numbers::pi_v<float>;
        v[i] = Vertex{ Vec3{ cx + r * std::cos(theta),
                              cy + r * std::sin(theta),
                              0.f },
                       col };
    }
    v[segments] = v[0]; // close

    m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor, col, ortho_mvp());
}

// ── submit_grid / submit_axes / submit_conics ─────────────────────────────────

void Scene::submit_grid() {
    const u32 count = math::grid_vertex_count(m_axes_cfg);
    if (count == 0) return;
    auto slice = m_api.acquire(count);
    math::build_grid({ slice.vertices(), count }, m_axes_cfg);
    m_api.submit(slice, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, ortho_mvp());
}

void Scene::submit_axes() {
    const u32 count = math::axes_vertex_count(m_axes_cfg);
    auto slice = m_api.acquire(count);
    math::build_axes({ slice.vertices(), count }, m_axes_cfg);
    m_api.submit(slice, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, ortho_mvp());
}

void Scene::submit_conics() {
    const Mat4 mvp = ortho_mvp();
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
    ImGui::SetNextWindowSize(ImVec2(300.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.88f);
    ImGui::Begin("Scene");

    ImGui::SeparatorText("Grid");
    ImGui::SliderFloat("Extent",     &m_axes_cfg.extent,     0.5f, 10.f, "%.1f");
    ImGui::SliderFloat("Major step", &m_axes_cfg.major_step, 0.1f, 2.f,  "%.2f");
    ImGui::SliderFloat("Minor step", &m_axes_cfg.minor_step, 0.05f, 1.f, "%.2f");
    ImGui::Checkbox("3D mode", &m_axes_cfg.is_3d);
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
        d |= ImGui::SliderFloat("a##p",     &e.par_a,    -4.f, 4.f,  "%.3f");
        d |= ImGui::SliderFloat("b##p",     &e.par_b,    -4.f, 4.f,  "%.3f");
        d |= ImGui::SliderFloat("c##p",     &e.par_c,    -2.f, 2.f,  "%.3f");
        d |= ImGui::SliderFloat("t min##p", &e.par_tmin, -4.f, 0.f,  "%.2f");
        d |= ImGui::SliderFloat("t max##p", &e.par_tmax,  0.f, 4.f,  "%.2f");
        if (ImGui::Button("Reset##p")) {
            e.par_a = 1.f; e.par_b = 0.f; e.par_c = 0.f;
            e.par_tmin = -1.5f; e.par_tmax = 1.5f; d = true;
        }
        if (d) e.mark_dirty();
        if (e.conic)
            ImGui::TextDisabled("k(0) = %.5f", e.conic->curvature(0.f));
    } else {
        ImGui::Text("(x-h)^2/a^2 - (y-k)^2/b^2 = 1");
        bool d = false;
        d |= ImGui::SliderFloat("a##h",     &e.hyp_a,     0.1f, 4.f,  "%.2f");
        d |= ImGui::SliderFloat("b##h",     &e.hyp_b,     0.1f, 4.f,  "%.2f");
        d |= ImGui::SliderFloat("h##h",     &e.hyp_h,    -2.f,  2.f,  "%.2f");
        d |= ImGui::SliderFloat("k##h",     &e.hyp_k,    -2.f,  2.f,  "%.2f");
        d |= ImGui::SliderFloat("range##h", &e.hyp_range, 0.5f, 4.f,  "%.2f");
        const char* axis_names[] = { "Horizontal", "Vertical" };
        if (ImGui::Combo("Axis##h", &e.hyp_axis, axis_names, 2)) d = true;
        ImGui::Checkbox("Two branches##h", &e.hyp_two_branch);
        if (ImGui::Button("Reset##h")) {
            e.hyp_a = 1.f; e.hyp_b = 1.f; e.hyp_h = 0.f;
            e.hyp_k = 0.f; e.hyp_range = 2.f; d = true;
        }
        if (d) e.mark_dirty();
    }

    ImGui::Unindent();
}

} // namespace ndde
