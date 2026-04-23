// app/GaussianSurface.cpp
#include "app/GaussianSurface.hpp"
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <numbers>
#include <glm/glm.hpp>

namespace ndde {

// ── GaussianSurface::eval ─────────────────────────────────────────────────────

f32 GaussianSurface::eval(f32 x, f32 y) noexcept {
    const f32 g0 =  1.2f * std::exp(-((x-1.5f)*(x-1.5f) + (y-0.5f)*(y-0.5f)));
    const f32 g1 = -0.9f * std::exp(-((x+1.2f)*(x+1.2f)/0.7f + (y+1.0f)*(y+1.0f)/1.4f));
    const f32 g2 =  0.7f * std::exp(-((x-0.3f)*(x-0.3f)/1.8f + (y-1.7f)*(y-1.7f)/0.6f));
    const f32 s  =  0.15f* std::sin(2.f*x) * std::sin(2.f*y);
    return g0 + g1 + g2 + s;
}

// ── GaussianSurface::grad ─────────────────────────────────────────────────────

GaussianSurface::Grad GaussianSurface::grad(f32 x, f32 y) noexcept {
    constexpr f32 h = 1e-3f;
    return {
        (eval(x+h,y) - eval(x-h,y)) / (2.f*h),
        (eval(x,y+h) - eval(x,y-h)) / (2.f*h)
    };
}

// ── GaussianSurface::unit_normal ──────────────────────────────────────────────

Vec3 GaussianSurface::unit_normal(f32 x, f32 y) noexcept {
    const auto [fx, fy] = grad(x, y);
    const f32 len = std::sqrt(fx*fx + fy*fy + 1.f);
    return { -fx/len, -fy/len, 1.f/len };
}

// ── GaussianSurface::gaussian_curvature ───────────────────────────────────────

f32 GaussianSurface::gaussian_curvature(f32 x, f32 y) noexcept {
    constexpr f32 h = 1e-3f;
    const auto [fx, fy] = grad(x, y);
    const f32 fxx = (eval(x+h,y) - 2.f*eval(x,y) + eval(x-h,y)) / (h*h);
    const f32 fyy = (eval(x,y+h) - 2.f*eval(x,y) + eval(x,y-h)) / (h*h);
    const f32 fxy = (eval(x+h,y+h) - eval(x+h,y-h) - eval(x-h,y+h) + eval(x-h,y-h)) / (4.f*h*h);
    const f32 E = 1.f+fx*fx, F = fx*fy, G = 1.f+fy*fy;
    const f32 n = std::sqrt(1.f + fx*fx + fy*fy);
    const f32 L = fxx/n, M = fxy/n, N2 = fyy/n;
    const f32 denom = E*G - F*F;
    return std::abs(denom) < 1e-10f ? 0.f : (L*N2 - M*M) / denom;
}

// ── GaussianSurface::mean_curvature ──────────────────────────────────────────

f32 GaussianSurface::mean_curvature(f32 x, f32 y) noexcept {
    constexpr f32 h = 1e-3f;
    const auto [fx, fy] = grad(x, y);
    const f32 fxx = (eval(x+h,y) - 2.f*eval(x,y) + eval(x-h,y)) / (h*h);
    const f32 fyy = (eval(x,y+h) - 2.f*eval(x,y) + eval(x,y-h)) / (h*h);
    const f32 fxy = (eval(x+h,y+h) - eval(x+h,y-h) - eval(x-h,y+h) + eval(x-h,y-h)) / (4.f*h*h);
    const f32 E = 1.f+fx*fx, F = fx*fy, G = 1.f+fy*fy;
    const f32 n = std::sqrt(1.f + fx*fx + fy*fy);
    const f32 L = fxx/n, M = fxy/n, N2 = fyy/n;
    const f32 denom = 2.f*(E*G - F*F);
    return std::abs(denom) < 1e-10f ? 0.f : (E*N2 + G*L - 2.f*F*M) / denom;
}

// ── GaussianSurface::height_color ─────────────────────────────────────────────

Vec4 GaussianSurface::height_color(f32 z) noexcept {
    const f32 t = std::clamp((z - Z_MIN) / (Z_MAX - Z_MIN), 0.f, 1.f);
    // 7-stop heatmap: deep-blue → blue → cyan → green → yellow → orange → red
    struct Stop { f32 pos; Vec3 rgb; };
    static constexpr Stop stops[] = {
        {0.00f, {0.165f, 0.000f, 0.431f}},
        {0.15f, {0.000f, 0.157f, 0.706f}},
        {0.35f, {0.000f, 0.627f, 0.784f}},
        {0.50f, {0.000f, 0.784f, 0.471f}},
        {0.65f, {0.784f, 0.863f, 0.000f}},
        {0.80f, {1.000f, 0.627f, 0.000f}},
        {1.00f, {0.784f, 0.000f, 0.118f}}
    };
    constexpr int N = 7;
    int lo = 0;
    for (int i = 0; i < N-1; ++i) {
        if (t >= stops[i].pos && t <= stops[i+1].pos) { lo = i; break; }
        lo = N-2;
    }
    const f32 span = stops[lo+1].pos - stops[lo].pos;
    const f32 dt   = span < 1e-6f ? 0.f : (t - stops[lo].pos) / span;
    const Vec3 c = stops[lo].rgb + dt*(stops[lo+1].rgb - stops[lo].rgb);
    return {c.x, c.y, c.z, 1.f};
}

// ── GaussianSurface::wireframe_vertex_count ───────────────────────────────────

u32 GaussianSurface::wireframe_vertex_count(u32 u_lines, u32 v_lines) noexcept {
    return (u_lines + 1u) * v_lines * 2u +
           (v_lines + 1u) * u_lines * 2u;
}

// ── GaussianSurface::tessellate_wireframe ─────────────────────────────────────

void GaussianSurface::tessellate_wireframe(std::span<Vertex> out,
                                            u32 u_lines, u32 v_lines) {
    const u32 needed = wireframe_vertex_count(u_lines, v_lines);
    if (out.size() < needed)
        throw std::invalid_argument("[GaussianSurface] span too small");

    const f32 xs = (XMAX - XMIN) / static_cast<f32>(u_lines);
    const f32 ys = (YMAX - YMIN) / static_cast<f32>(v_lines);
    u32 idx = 0;

    // Lines of constant x (vary y)
    for (u32 i = 0; i <= u_lines; ++i) {
        const f32 x = XMIN + static_cast<f32>(i) * xs;
        for (u32 j = 0; j < v_lines; ++j) {
            const f32 ya = YMIN + static_cast<f32>(j)   * ys;
            const f32 yb = YMIN + static_cast<f32>(j+1) * ys;
            const f32 za = eval(x, ya);
            const f32 zb = eval(x, yb);
            out[idx++] = { Vec3{x, ya, za}, height_color(za) };
            out[idx++] = { Vec3{x, yb, zb}, height_color(zb) };
        }
    }
    // Lines of constant y (vary x)
    for (u32 j = 0; j <= v_lines; ++j) {
        const f32 y = YMIN + static_cast<f32>(j) * ys;
        for (u32 i = 0; i < u_lines; ++i) {
            const f32 xa = XMIN + static_cast<f32>(i)   * xs;
            const f32 xb = XMIN + static_cast<f32>(i+1) * xs;
            const f32 za = eval(xa, y);
            const f32 zb = eval(xb, y);
            out[idx++] = { Vec3{xa, y, za}, height_color(za) };
            out[idx++] = { Vec3{xb, y, zb}, height_color(zb) };
        }
    }
}

// ── GaussianSurface::contour_max_vertices ────────────────────────────────────

u32 GaussianSurface::contour_max_vertices(u32 grid_n, u32 n_levels) noexcept {
    return grid_n * grid_n * 4u * n_levels;
}

// ── GaussianSurface::tessellate_contours ─────────────────────────────────────
// Marching-squares to extract level sets as line segments (LineList).

u32 GaussianSurface::tessellate_contours(std::span<Vertex> out,
                                          u32 grid_n,
                                          const float* levels, u32 n_levels,
                                          Vec4 color) {
    u32 idx = 0;
    const f32 dx = (XMAX-XMIN) / static_cast<f32>(grid_n);
    const f32 dy = (YMAX-YMIN) / static_cast<f32>(grid_n);

    for (u32 li = 0; li < n_levels; ++li) {
        const f32 lv = levels[li];
        for (u32 i = 0; i < grid_n; ++i) {
            for (u32 j = 0; j < grid_n; ++j) {
                const f32 x0 = XMIN + static_cast<f32>(i)   * dx;
                const f32 x1 = XMIN + static_cast<f32>(i+1) * dx;
                const f32 y0 = YMIN + static_cast<f32>(j)   * dy;
                const f32 y1 = YMIN + static_cast<f32>(j+1) * dy;
                const f32 v00 = eval(x0,y0), v10 = eval(x1,y0);
                const f32 v01 = eval(x0,y1), v11 = eval(x1,y1);

                // Collect crossing points on the 4 edges
                f32 pts[8]; u32 np = 0;
                auto seg = [&](f32 va, f32 vb, f32 xa, f32 ya, f32 xb, f32 yb) {
                    if ((va-lv)*(vb-lv) < 0.f) {
                        const f32 t = (lv-va)/(vb-va);
                        pts[np++] = xa + t*(xb-xa);
                        pts[np++] = ya + t*(yb-ya);
                    }
                };
                seg(v00,v10, x0,y0, x1,y0);
                seg(v10,v11, x1,y0, x1,y1);
                seg(v11,v01, x1,y1, x0,y1);
                seg(v01,v00, x0,y1, x0,y0);

                if (np >= 4 && idx+2 <= out.size()) {
                    out[idx++] = { Vec3{pts[0], pts[1], 0.f}, color };
                    out[idx++] = { Vec3{pts[2], pts[3], 0.f}, color };
                }
            }
        }
    }
    return idx;
}

// ═══════════════════════════════════════════════════════════════════════════════
// AnimatedCurve
// ═══════════════════════════════════════════════════════════════════════════════

void AnimatedCurve::reset() {
    m_trail.clear();
    m_walk = WalkState{};
}

void AnimatedCurve::advance(f32 dt, f32 speed_scale) {
    const int sub_steps = 4;
    const f32 sub_dt    = dt / static_cast<f32>(sub_steps);
    for (int s = 0; s < sub_steps; ++s)
        step(sub_dt, speed_scale);
}

void AnimatedCurve::step(f32 dt, f32 speed_scale) {
    f32& x     = m_walk.x;
    f32& y     = m_walk.y;
    f32& phase = m_walk.phase;
    f32& angle = m_walk.angle;

    // Walk direction: mostly perpendicular to gradient (level-curve following)
    // with a slow sinusoidal steering component so it crosses the surface.
    const auto [fx, fy] = GaussianSurface::grad(x, y);
    const f32 gn = std::sqrt(fx*fx + fy*fy) + 1e-5f;

    // Perpendicular to gradient (level-curve tangent)
    const f32 perp_x = -fy / gn;
    const f32 perp_y =  fx / gn;

    // Steer toward desired angle
    const f32 steer   = std::sin(phase * 0.4f) * 0.55f;
    const f32 desired = std::atan2(perp_y, perp_x) + steer;
    // Smooth angle tracking
    f32 da = desired - angle;
    while (da >  std::numbers::pi_v<f32>) da -= 2.f*std::numbers::pi_v<f32>;
    while (da < -std::numbers::pi_v<f32>) da += 2.f*std::numbers::pi_v<f32>;
    angle += std::clamp(da, -2.f*dt, 2.f*dt);

    const f32 sp = WALK_SPEED * speed_scale;
    f32 nx = x + std::cos(angle) * sp * dt;
    f32 ny = y + std::sin(angle) * sp * dt;

    // Soft boundary reflection
    const f32 margin = 0.3f;
    if (nx < GaussianSurface::XMIN + margin) { nx = GaussianSurface::XMIN + margin; angle = std::numbers::pi_v<f32> - angle; }
    if (nx > GaussianSurface::XMAX - margin) { nx = GaussianSurface::XMAX - margin; angle = std::numbers::pi_v<f32> - angle; }
    if (ny < GaussianSurface::YMIN + margin) { ny = GaussianSurface::YMIN + margin; angle = -angle; }
    if (ny > GaussianSurface::YMAX - margin) { ny = GaussianSurface::YMAX - margin; angle = -angle; }

    x = nx; y = ny;
    phase += sp * dt * 1.5f;

    const f32 z = GaussianSurface::eval(x, y);
    const Vec3 pt = {x, y, z};

    if (m_trail.empty() || glm::length(pt - m_trail.back()) > 0.015f) {
        m_trail.push_back(pt);
        if (m_trail.size() > MAX_TRAIL)
            m_trail.erase(m_trail.begin());
    }
}

FrenetFrame AnimatedCurve::frenet_at(u32 idx) const noexcept {
    FrenetFrame fr;
    const u32 n = static_cast<u32>(m_trail.size());
    if (n < 3 || idx < 1 || idx >= n-1) return fr;

    const Vec3& pm = m_trail[idx-1];
    const Vec3& p0 = m_trail[idx];
    const Vec3& pp = m_trail[idx+1];

    const Vec3 v1 = p0 - pm;
    const Vec3 v2 = pp - p0;
    const f32  l1 = glm::length(v1);
    const f32  l2 = glm::length(v2);
    if (l1 < 1e-9f || l2 < 1e-9f) return fr;

    const Vec3 T1 = v1 / l1;
    const Vec3 T2 = v2 / l2;
    fr.T = glm::normalize(T1 + T2);  // average tangent
    fr.speed = (l1 + l2) * 0.5f;

    const Vec3 dT  = T2 - T1;
    const f32  dTl = glm::length(dT);
    if (dTl > 1e-9f) {
        fr.N     = dT / dTl;
        fr.kappa = dTl / l1;
    } else {
        // Degenerate: use surface normal as N
        fr.N     = GaussianSurface::unit_normal(p0.x, p0.y);
        fr.kappa = 0.f;
    }
    fr.B = glm::normalize(glm::cross(fr.T, fr.N));

    // Torsion: (T1 x T2) . (T_prev x T_curr) / ...
    if (idx >= 2 && idx < n-2) {
        const Vec3& ppp = m_trail[idx+2];
        const Vec3  v3  = ppp - pp;
        const f32   l3  = glm::length(v3);
        if (l3 > 1e-9f) {
            const Vec3 T3    = v3 / l3;
            const Vec3 cross = glm::cross(T2, T3);
            const f32  clen  = glm::length(cross);
            fr.tau = clen / (l2 + 1e-9f);
        }
    }
    return fr;
}

u32 AnimatedCurve::trail_vertex_count() const noexcept {
    return static_cast<u32>(m_trail.size());
}

void AnimatedCurve::tessellate_trail(std::span<Vertex> out) const {
    const u32 n = trail_vertex_count();
    if (out.size() < n) return;
    // Colour trail by age: older points fade to dark, head is white
    for (u32 i = 0; i < n; ++i) {
        const f32 t     = static_cast<f32>(i) / static_cast<f32>(n > 1 ? n-1 : 1);
        const Vec4 col  = {t, t*0.8f + 0.2f, 1.f, 0.3f + 0.7f*t};
        out[i] = { m_trail[i], col };
    }
}

Vec3 AnimatedCurve::head_world() const noexcept {
    if (m_trail.empty()) return {m_walk.x, m_walk.y, GaussianSurface::eval(m_walk.x, m_walk.y)};
    return m_trail.back();
}

} // namespace ndde
