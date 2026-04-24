#pragma once
// math/Surfaces.hpp
// Parametric surface interface and concrete 2-manifolds.
//
// Design:
//   ISurface defines a smooth map  f : R² → R³,  (u,v) → p(u,v).
//   Concrete classes implement evaluate(u,v) and the first partial derivatives.
//   tessellate_wireframe() writes a UV grid as line segments (LineList) for
//   the current implementation — surfaces are rendered as wireframes so the
//   underlying curve geometry remains visible.
//
// Geometric intuition:
//   A 2-manifold embedded in R³ is locally homeomorphic to R².
//   The paraboloid  z = x² + y²  is the simplest non-flat example:
//     - It is smooth everywhere (C∞)
//     - It has non-zero, varying Gaussian curvature
//     - It is the graph of a function, so it has a natural (u,v) chart
//     - Vertical-plane cross-sections are parabolas lying on the surface —
//       making the curve-on-manifold relationship concrete and visualisable.
//
// See math/Conics.hpp for IConic curves that lie on these surfaces.
// See app/Scene for rendering wiring.

#include "math/Scalars.hpp"
#include "renderer/GpuTypes.hpp"
#include <span>
#include <stdexcept>
#include <cmath>

namespace ndde::math {

// ── ISurface ──────────────────────────────────────────────────────────────────
// A smooth parametric surface  p : [u_min, u_max] × [v_min, v_max] → R³.
//
// Partial derivatives are virtual and default to central-difference finite
// differences; override with analytic formulas when available.

class ISurface {
public:
    virtual ~ISurface() = default;

    [[nodiscard]] virtual Vec3  evaluate(float u, float v)    const = 0;
    [[nodiscard]] virtual float u_min() const = 0;
    [[nodiscard]] virtual float u_max() const = 0;
    [[nodiscard]] virtual float v_min() const = 0;
    [[nodiscard]] virtual float v_max() const = 0;

    // First-order partial derivatives (default: finite difference)
    [[nodiscard]] virtual Vec3 du(float u, float v) const;
    [[nodiscard]] virtual Vec3 dv(float u, float v) const;

    // Unit normal  N = (du × dv) / |du × dv|
    [[nodiscard]] Vec3 unit_normal(float u, float v) const;

    // Gaussian curvature K = (LN - M²) / (EG - F²)
    // (second fundamental form coefficients via finite differences)
    [[nodiscard]] float gaussian_curvature(float u, float v) const;

    // Mean curvature H = (EN + GL - 2FM) / (2(EG - F²))
    [[nodiscard]] float mean_curvature(float u, float v) const;

    // Wireframe tessellation:
    //   u_steps × v_steps parameter-space grid of line segments.
    //   Each grid line goes in one direction (u-lines and v-lines separately).
    //   Total vertices: 2 * (u_steps+1) * v_lines + 2 * u_lines * (v_steps+1)
    [[nodiscard]] u32 wireframe_vertex_count(u32 u_lines, u32 v_lines) const noexcept;

    void tessellate_wireframe(std::span<Vertex> out,
                              u32 u_lines, u32 v_lines,
                              Vec4 color = { 1.f, 1.f, 1.f, 1.f }) const;
};

// ── Paraboloid ────────────────────────────────────────────────────────────────
// The circular paraboloid  z = a(x² + y²),  parameterised in polar form:
//
//   p(u, v) = (u·cos v,  u·sin v,  a·u²)
//
//   u ∈ [0, u_max]   — radial coordinate (distance from axis)
//   v ∈ [0, 2π]      — angular coordinate
//   a > 0            — curvature scale (a=1 gives the unit paraboloid)
//
// Differential geometry (all analytic):
//
//   ∂p/∂u = (cos v,         sin v,        2au)
//   ∂p/∂v = (-u·sin v,      u·cos v,      0  )
//
//   First fundamental form coefficients:
//     E = g₁₁ = |∂p/∂u|² = 1 + 4a²u²
//     F = g₁₂ = ∂p/∂u · ∂p/∂v = 0          (orthogonal coordinate lines)
//     G = g₂₂ = |∂p/∂v|² = u²
//
//   Unit normal:
//     N = (∂p/∂u × ∂p/∂v) / |∂p/∂u × ∂p/∂v|
//       = (-2au·cos v, -2au·sin v, u) / (u·√(1 + 4a²u²))
//
//   Second fundamental form:
//     L =  2a / √(1 + 4a²u²)
//     M =  0
//     N̂ = 2au² / √(1 + 4a²u²)         (using N̂ to avoid clash with unit normal)
//
//   Principal curvatures:
//     κ₁ = L/E = 2a / (1 + 4a²u²)^(3/2)
//     κ₂ = N̂/G = 2a / √(1 + 4a²u²)
//
//   Gaussian curvature:
//     K = κ₁·κ₂ = 4a² / (1 + 4a²u²)²
//       Always positive → paraboloid is a convex surface.
//       K → 0 as u → ∞ (flattens out at large radius)
//       K = 4a² at u=0 (maximum curvature at the vertex)
//
//   Mean curvature:
//     H = (κ₁ + κ₂)/2 = a(2 + 4a²u²) / (1 + 4a²u²)^(3/2)
//
// The paraboloid is the boundary of the standard parabolic bowl used in
// optics and radar, and is the zero-set of a Morse function — making it
// a textbook example of a smooth manifold with a clean chart atlas.

class Paraboloid final : public ISurface {
public:
    /// a       = curvature scale  (z = a(x² + y²))
    /// u_max   = maximum radial extent
    /// v_min/max = angular range (default: full circle 0 → 2π)
    explicit Paraboloid(float a     = 1.f,
                        float u_max = 1.5f,
                        float vmin  = 0.f,
                        float vmax  = 6.2831853f);  // 2π

    [[nodiscard]] Vec3  evaluate(float u, float v) const override;
    [[nodiscard]] Vec3  du(float u, float v)        const override;
    [[nodiscard]] Vec3  dv(float u, float v)        const override;

    [[nodiscard]] float u_min() const override { return 0.f;    }
    [[nodiscard]] float u_max() const override { return m_umax; }
    [[nodiscard]] float v_min() const override { return m_vmin; }
    [[nodiscard]] float v_max() const override { return m_vmax; }

    [[nodiscard]] float a()     const noexcept { return m_a; }

    // Analytic overrides
    [[nodiscard]] Vec3  unit_normal(float u, float v)      const;
    [[nodiscard]] float gaussian_curvature(float u, float v) const;
    [[nodiscard]] float mean_curvature(float u, float v)   const;

    // κ₁, κ₂ at (u,v)
    [[nodiscard]] float kappa1(float u) const noexcept;
    [[nodiscard]] float kappa2(float u) const noexcept;

private:
    float m_a, m_umax, m_vmin, m_vmax;
};

} // namespace ndde::math
