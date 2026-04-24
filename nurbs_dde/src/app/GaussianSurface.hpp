#pragma once
// app/GaussianSurface.hpp
// Option C surface: 6-Gaussian asymmetric + double sinusoidal ripple.
// Domain: [-6,6]x[-6,6].  Z range: approx [-2.0, +2.2].
//
// ── What this provides ────────────────────────────────────────────────────────
// 1. f(x,y)   — scalar surface height (Option C)
// 2. grad     — gradient via central FD
// 3. K, H     — Gaussian / mean curvature via second fundamental form
// 4. SurfaceFrame — coordinate tangent frame + κ_n / κ_g decomposition
// 5. AnimatedCurve — one walker particle; SurfaceSimScene owns a vector of them
// 6. Wireframe + contour tessellation for the two views

#include "math/Scalars.hpp"
#include "renderer/GpuTypes.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <span>
#include <functional>

namespace ndde {

// ── GaussianSurface ───────────────────────────────────────────────────────────

class GaussianSurface {
public:
    static constexpr f32 XMIN = -6.f;
    static constexpr f32 XMAX =  6.f;
    static constexpr f32 YMIN = -6.f;
    static constexpr f32 YMAX =  6.f;

    [[nodiscard]] static f32  eval(f32 x, f32 y) noexcept;

    struct Grad { f32 fx, fy; };
    [[nodiscard]] static Grad grad(f32 x, f32 y) noexcept;
    [[nodiscard]] static Vec3 unit_normal(f32 x, f32 y) noexcept;
    [[nodiscard]] static f32  gaussian_curvature(f32 x, f32 y) noexcept;
    [[nodiscard]] static f32  mean_curvature(f32 x, f32 y) noexcept;

    [[nodiscard]] static u32 wireframe_vertex_count(u32 u_lines, u32 v_lines) noexcept;
    static void tessellate_wireframe(std::span<Vertex> out, u32 u_lines, u32 v_lines);

    [[nodiscard]] static u32 contour_max_vertices(u32 grid_n, u32 n_levels) noexcept;
    static u32 tessellate_contours(std::span<Vertex> out,
                                   u32 grid_n,
                                   const float* levels, u32 n_levels,
                                   Vec4 color = {1.f,1.f,1.f,0.7f});

    [[nodiscard]] static Vec4 height_color(f32 z) noexcept;

    static constexpr f32 Z_MIN = -2.0f;
    static constexpr f32 Z_MAX =  2.2f;
};

// ── FrenetFrame ───────────────────────────────────────────────────────────────

struct FrenetFrame {
    Vec3 T = {1.f, 0.f, 0.f};  ///< unit tangent
    Vec3 N = {0.f, 1.f, 0.f};  ///< principal normal
    Vec3 B = {0.f, 0.f, 1.f};  ///< binormal
    f32  kappa = 0.f;            ///< curvature κ
    f32  tau   = 0.f;            ///< torsion τ
    f32  speed = 0.f;            ///< |p'(t)|
};

// ── SurfaceFrame ──────────────────────────────────────────────────────────────
// Coordinate tangent frame of p(x,y) = (x,y,f(x,y)) at a foot-point.
//
//   Dx = ∂p/∂x = (1, 0, f_x)   |Dx|² = E
//   Dy = ∂p/∂y = (0, 1, f_y)   |Dy|² = G     Dx·Dy = F
//   n  = unit surface normal
//
// With FrenetFrame:
//   κ_n = κ·(N·n)                  normal curvature   (surface property)
//   κ_g = κ·|N − (N·n)n|           geodesic curvature (curve-on-surface)
//   κ²  = κ_n² + κ_g²              Meusnier–Pythagorean identity

struct SurfaceFrame {
    Vec3 Dx;              ///< ∂p/∂x — unnormalized
    Vec3 Dy;              ///< ∂p/∂y — unnormalized
    Vec3 normal;          ///< unit surface normal
    f32  E = 0.f;         ///< |Dx|²
    f32  F = 0.f;         ///< Dx·Dy
    f32  G = 0.f;         ///< |Dy|²
    f32  kappa_n = 0.f;   ///< normal curvature in T-direction
    f32  kappa_g = 0.f;   ///< geodesic curvature
};

[[nodiscard]] inline SurfaceFrame
make_surface_frame(f32 x, f32 y, const FrenetFrame* fr = nullptr) noexcept
{
    const auto [fx, fy] = GaussianSurface::grad(x, y);
    SurfaceFrame sf;
    sf.Dx     = Vec3{1.f, 0.f, fx};
    sf.Dy     = Vec3{0.f, 1.f, fy};
    sf.E      = glm::dot(sf.Dx, sf.Dx);
    sf.F      = glm::dot(sf.Dx, sf.Dy);
    sf.G      = glm::dot(sf.Dy, sf.Dy);
    sf.normal = GaussianSurface::unit_normal(x, y);
    if (fr && fr->kappa > 1e-6f) {
        const f32  NdotN = glm::dot(fr->N, sf.normal);
        sf.kappa_n = fr->kappa * NdotN;
        sf.kappa_g = fr->kappa * glm::length(fr->N - NdotN * sf.normal);
    }
    return sf;
}

// ── AnimatedCurve ─────────────────────────────────────────────────────────────
// One walker particle on the surface.
// SurfaceSimScene owns a std::vector<AnimatedCurve>.
// Ctrl+L appends a new one at a caller-specified start position.
// colour_id (0-5) selects the trail hue so particles are distinguishable.

class AnimatedCurve {
public:
    static constexpr u32  MAX_TRAIL   = 1200;
    static constexpr f32  WALK_SPEED  = 0.9f;
    static constexpr u32  MAX_COLOURS = 6u;

    explicit AnimatedCurve(f32 start_x = -4.5f, f32 start_y = -4.0f,
                           u32 colour_id = 0);

    void reset();
    void advance(f32 dt, f32 speed_scale = 1.f);

    [[nodiscard]] FrenetFrame frenet_at(u32 idx) const noexcept;

    [[nodiscard]] u32  trail_vertex_count() const noexcept;
    void tessellate_trail(std::span<Vertex> out) const;
    [[nodiscard]] Vec3 head_world() const noexcept;

    [[nodiscard]] u32  trail_size()    const noexcept { return static_cast<u32>(m_trail.size()); }
    [[nodiscard]] bool has_trail()     const noexcept { return m_trail.size() >= 3; }
    [[nodiscard]] Vec3 trail_pt(u32 i) const noexcept { return m_trail[i]; }
    [[nodiscard]] u32  colour_id()     const noexcept { return m_colour_id; }

    // Trail colour: age_t ∈ [0,1] (0=oldest, 1=head).
    [[nodiscard]] static Vec4 trail_colour(u32 id, f32 age_t) noexcept;

private:
    struct WalkState { f32 x, y, phase = 0.f, angle = 0.f; };

    WalkState         m_walk;
    std::vector<Vec3> m_trail;
    u32               m_colour_id;
    f32               m_start_x, m_start_y;

    void step(f32 dt, f32 speed_scale);
};

} // namespace ndde
