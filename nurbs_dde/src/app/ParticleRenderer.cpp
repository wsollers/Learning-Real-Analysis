// app/ParticleRenderer.cpp
// All submit_* methods moved from SurfaceSimScene (B2 refactor).
// submit_all() consolidates the per-curve rendering loop that previously
// lived inside draw_surface_3d_window().

#include "app/ParticleRenderer.hpp"
#include "numeric/ops.hpp"
#include <imgui.h>
#include <glm/glm.hpp>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace ndde {

// ── submit_arrow ──────────────────────────────────────────────────────────────

void ParticleRenderer::submit_arrow(Vec3 origin, Vec3 dir, Vec4 color,
                                     float length, const Mat4& mvp, Topology topo)
{
    auto slice = m_api.acquire(2);
    Vertex* v  = slice.vertices();
    v[0] = { origin,              color };
    v[1] = { origin + dir*length, color };
    m_api.submit_to(RenderTarget::Primary3D, slice, topo, DrawMode::VertexColor, color, mvp);
}

// ── submit_trail_3d ───────────────────────────────────────────────────────────

void ParticleRenderer::submit_trail_3d(const AnimatedCurve& c, const Mat4& mvp) {
    const u32 n = c.trail_vertex_count();
    if (n < 2) return;
    auto slice = m_api.acquire(n);
    c.tessellate_trail({slice.vertices(), n});
    m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineStrip, DrawMode::VertexColor, {1,1,1,1}, mvp);
}

// ── submit_head_dot_3d ────────────────────────────────────────────────────────

void ParticleRenderer::submit_head_dot_3d(const AnimatedCurve& c, const Mat4& mvp) {
    const Vec3 hp  = c.head_world();
    const Vec4 col = c.head_colour();
    auto slice = m_api.acquire(1);
    slice.vertices()[0] = { hp, col };
    m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineStrip, DrawMode::UniformColor, col, mvp);
}

// ── submit_frenet_3d ──────────────────────────────────────────────────────────

void ParticleRenderer::submit_frenet_3d(const AnimatedCurve& c, u32 trail_idx,
                                         const Mat4& mvp) {
    if (!c.has_trail() || trail_idx == 0) return;
    const FrenetFrame fr = c.frenet_at(trail_idx);
    const Vec3 o = c.trail_pt(trail_idx);
    if (show_T) submit_arrow(o, fr.T, {1.f,0.35f,0.05f,1.f}, frame_scale, mvp);
    if (show_N) submit_arrow(o, fr.N, {0.15f,1.f,0.3f,1.f},  frame_scale, mvp);
    if (show_B) submit_arrow(o, fr.B, {0.2f,0.5f,1.f,1.f},   frame_scale, mvp);
}

// ── submit_osc_circle_3d ─────────────────────────────────────────────────────

void ParticleRenderer::submit_osc_circle_3d(const AnimatedCurve& c, u32 trail_idx,
                                              const Mat4& mvp) {
    if (!c.has_trail() || trail_idx == 0) return;
    const FrenetFrame fr = c.frenet_at(trail_idx);
    if (fr.kappa < 1e-5f) return;
    const float R      = 1.f / fr.kappa;
    const Vec3  o      = c.trail_pt(trail_idx);
    const Vec3  centre = o + fr.N * R;
    constexpr u32 SEG = 64;
    const Vec4 col = {0.7f,0.3f,1.f,0.75f};
    auto slice = m_api.acquire(SEG+1);
    Vertex* v  = slice.vertices();
    for (u32 i = 0; i <= SEG; ++i) {
        const float theta = (static_cast<float>(i)/SEG)*ops::two_pi_v<float>;
        v[i] = { centre + R*(-ops::cos(theta)*fr.N + ops::sin(theta)*fr.T), col };
    }
    m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineStrip, DrawMode::VertexColor, col, mvp);
}

// ── submit_surface_frame_3d ───────────────────────────────────────────────────
// Dx cyan, Dy magenta, D_T (directional derivative in T) yellow.
// Now takes explicit surface + sim_time instead of reading scene members.

void ParticleRenderer::submit_surface_frame_3d(const AnimatedCurve&        c,
                                                u32                         trail_idx,
                                                const Mat4&                 mvp,
                                                const ndde::math::ISurface& surface,
                                                float                       sim_time) {
    if (!c.has_trail() || trail_idx == 0) return;
    const Vec3 p  = c.trail_pt(trail_idx);
    const SurfaceFrame sf = make_surface_frame(surface, p.x, p.y, sim_time);
    const float scale = frame_scale;

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
// Now takes explicit surface + sim_time instead of reading scene members.

void ParticleRenderer::submit_normal_plane_3d(const AnimatedCurve&        c,
                                               u32                         trail_idx,
                                               const Mat4&                 mvp,
                                               const ndde::math::ISurface& surface,
                                               float                       sim_time) {
    if (!c.has_trail() || trail_idx == 0) return;
    const Vec3        p   = c.trail_pt(trail_idx);
    const FrenetFrame fr  = c.frenet_at(trail_idx);
    const SurfaceFrame sf = make_surface_frame(surface, p.x, p.y, sim_time, &fr);

    const float osc_r  = (fr.kappa > 1e-5f) ? 1.f / fr.kappa : 2.f;
    const float half_T = std::clamp(osc_r * 0.3f * frame_scale, 0.04f, 1.2f);
    const float half_n = std::clamp(ops::sqrt(sf.E) * frame_scale, 0.04f, 1.2f);

    const Vec3 a = fr.T      * half_T;
    const Vec3 b = sf.normal * half_n;
    const Vec3 corners[5] = { p-a-b, p+a-b, p+a+b, p-a+b, p-a-b };

    const Vec4 col = {0.9f, 0.6f, 0.1f, 0.85f};
    auto slice = m_api.acquire(5);
    Vertex* v  = slice.vertices();
    for (u32 i = 0; i < 5; ++i) v[i] = { corners[i], col };
    m_api.submit_to(RenderTarget::Primary3D, slice, Topology::LineStrip, DrawMode::VertexColor, col, mvp);

    auto diag = m_api.acquire(2);
    diag.vertices()[0] = { p-a+b, {col.r, col.g, col.b, 0.5f} };
    diag.vertices()[1] = { p+a-b, {col.r, col.g, col.b, 0.5f} };
    m_api.submit_to(RenderTarget::Primary3D, diag, Topology::LineList, DrawMode::VertexColor, col, mvp);
}

// ── submit_torsion_3d ─────────────────────────────────────────────────────────
// Geometric meaning: dB/ds = -tau*N (third Frenet-Serret equation).
// Visualisation: dB/ds arrow + twist fan in the osculating plane.
// Colour: orange (tau > 0, right twist), cyan (tau < 0, left twist).

void ParticleRenderer::submit_torsion_3d(const AnimatedCurve& c, u32 trail_idx,
                                          const Mat4& mvp) {
    if (!c.has_trail() || trail_idx == 0) return;
    const FrenetFrame fr = c.frenet_at(trail_idx);
    if (fr.kappa < 1e-5f) return;

    const Vec3  o   = c.trail_pt(trail_idx);
    const float tau = fr.tau;
    const float scl = frame_scale;

    const Vec4 col = tau > 0.f
        ? Vec4{1.f, 0.55f, 0.1f, 0.9f}    // orange: positive torsion
        : Vec4{0.3f, 0.8f, 1.f,  0.9f};   // cyan:   negative torsion

    // (1) dB/ds arrow — direction is -tau*N
    const float dB_len = std::clamp(std::abs(tau) * scl * 2.f, 0.02f, 1.5f);
    const Vec3  dB_dir = fr.N * (tau > 0.f ? -1.f : 1.f);
    submit_arrow(o, dB_dir, col, dB_len, mvp);

    // (2) Twist fan in the osculating plane
    if (std::abs(tau) < 1e-4f) return;

    const float helix_angle = ops::atan(std::abs(tau) / fr.kappa);
    const float fan_angle   = std::clamp(helix_angle, 0.01f, ops::pi_v<float> / 3.f);
    const float fan_r       = scl * 0.7f;
    constexpr u32 FAN_SEG   = 20;
    const Vec3 side = tau > 0.f ? -fr.B : fr.B;

    auto fan = m_api.acquire(FAN_SEG + 2);
    Vertex* vf = fan.vertices();
    vf[0] = { o, col };
    for (u32 i = 0; i <= FAN_SEG; ++i) {
        const float a    = (static_cast<float>(i) / FAN_SEG) * fan_angle;
        const Vec3  spoke = ops::cos(a) * fr.N + ops::sin(a) * side;
        vf[i + 1] = { o + spoke * fan_r, {col.r, col.g, col.b, 0.45f} };
    }
    m_api.submit_to(RenderTarget::Primary3D, fan, Topology::TriangleList, DrawMode::VertexColor, col, mvp);

    auto outline = m_api.acquire(FAN_SEG + 2);
    Vertex* vo   = outline.vertices();
    vo[0] = { o, col };
    for (u32 i = 0; i <= FAN_SEG; ++i) {
        const float a   = (static_cast<float>(i) / FAN_SEG) * fan_angle;
        const Vec3  sp  = ops::cos(a) * fr.N + ops::sin(a) * side;
        vo[i + 1] = { o + sp * fan_r, col };
    }
    m_api.submit_to(RenderTarget::Primary3D, outline, Topology::LineStrip, DrawMode::VertexColor, col, mvp);
}

// ── submit_all ────────────────────────────────────────────────────────────────
// The per-curve rendering loop, moved from draw_surface_3d_window().
// Submits: trail + head dot for every curve; overlays for the active curve.

void ParticleRenderer::submit_all(const std::vector<AnimatedCurve>& curves,
                                   const ndde::math::ISurface&        surface,
                                   float                               sim_time,
                                   const Mat4&                         mvp,
                                   int  snap_curve,
                                   int  snap_idx,
                                   bool snap_on_curve)
{
    for (u32 ci = 0; ci < static_cast<u32>(curves.size()); ++ci) {
        const AnimatedCurve& c = curves[ci];
        if (!c.has_trail()) continue;
        submit_trail_3d(c, mvp);
        submit_head_dot_3d(c, mvp);

        // Overlays on the snapped curve, or curve 0 when nothing is snapped.
        const bool is_active = (snap_on_curve && snap_curve == static_cast<int>(ci))
                            || (!snap_on_curve && ci == 0);
        const u32 sidx = (snap_on_curve && snap_curve == static_cast<int>(ci)
                          && snap_idx >= 0)
            ? static_cast<u32>(snap_idx)
            : (c.trail_size() > 2 ? c.trail_size()-2 : 0);

        if (show_frenet) {
            submit_frenet_3d(c, sidx, mvp);
            if (show_osc) submit_osc_circle_3d(c, sidx, mvp);
        }

        if (!is_active) continue;
        if (show_dir_deriv)    submit_surface_frame_3d(c, sidx, mvp, surface, sim_time);
        if (show_normal_plane) submit_normal_plane_3d(c, sidx, mvp, surface, sim_time);
        if (show_torsion)      submit_torsion_3d(c, sidx, mvp);
    }
}

} // namespace ndde
