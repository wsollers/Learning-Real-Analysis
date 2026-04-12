#pragma once
// math/Conics.hpp
// Parametric curve interface and concrete conic sections.
//
// Design contract
// ───────────────
// IConic is purely mathematical — no Vulkan, no heap allocation for
// vertices. It describes a curve as a smooth map p: [t_min, t_max] -> R^3.
// tessellate() writes into a caller-supplied span<Vertex>. The caller
// (Scene) acquires a slice from BufferManager first, then passes the
// mapped pointer here. Zero-copy: math writes directly into GPU-visible memory.
// derivative() and second_derivative() are implemented via central
// finite differences by default. Concrete subclasses should override
// with analytic forms for correctness in the analysis panels.
// curvature() has a default implementation via the Frenet formula:
//     k(t) = |p'(t) x p''(t)| / |p'(t)|^3
//
// Formal predicate (for predicates.yaml):
//   IConic is the type-level witness that a concrete curve satisfies:
//     forall t in [t_min, t_max] subset R, evaluate(t) in R^3
//   and that evaluate is at least C^1 on the open interval (t_min, t_max).

#include "math/Types.hpp"
#include <span>
#include <stdexcept>
#include <cmath>
#include <numbers>

namespace ndde::math {

// ── IConic ────────────────────────────────────────────────────────────────────

class IConic {
public:
    virtual ~IConic() = default;

    // ── Core evaluate ─────────────────────────────────────────────────────────

    /// Position on the curve at parameter t.
    /// Pre-condition: t in [t_min(), t_max()].
    [[nodiscard]] virtual Vec3 evaluate(float t) const = 0;

    [[nodiscard]] virtual float t_min() const = 0;
    [[nodiscard]] virtual float t_max() const = 0;

    // ── Differential geometry ─────────────────────────────────────────────────

    /// First derivative p'(t) — velocity vector.
    /// Default: central finite difference with h = (t_max - t_min) * 1e-4.
    [[nodiscard]] virtual Vec3 derivative(float t) const;

    /// Second derivative p''(t) — acceleration vector.
    [[nodiscard]] virtual Vec3 second_derivative(float t) const;

    /// Third derivative p'''(t) — jerk.
    [[nodiscard]] virtual Vec3 third_derivative(float t) const;

    /// Signed curvature k(t) = |p' x p''| / |p'|^3.
    /// Returns 0 for a straight line (degenerate cross product).
    [[nodiscard]] virtual float curvature(float t) const;

    /// Unit tangent T(t) = p'(t) / |p'(t)|.
    [[nodiscard]] Vec3 unit_tangent(float t) const;

    // ── Tessellation ──────────────────────────────────────────────────────────

    /// Write n+1 uniformly-spaced samples into out[0..n].
    /// out must have capacity >= n + 1. Color is applied uniformly.
    /// This writes directly into GPU-mapped memory — no allocation.
    void tessellate(std::span<Vertex> out,
                    u32               n,
                    Vec4              color = { 1.f, 1.f, 1.f, 1.f }) const;

    /// Convenience: returns the required vertex count for tessellate().
    [[nodiscard]] u32 vertex_count(u32 n) const noexcept { return n + 1; }

private:
    [[nodiscard]] float h(float t_span) const noexcept {
        return t_span * 1e-4f;
    }
};

// ── Parabola ──────────────────────────────────────────────────────────────────
// p(t) = (t,  a*t^2 + b*t + c,  0)   t in [t_min, t_max]
//
// Predicate: Parabola(a,b,c) evaluates to the unique parabola with
//   vertex at (-b/2a, c - b^2/4a) and opening direction sgn(a)*y_hat.

class Parabola final : public IConic {
public:
    explicit Parabola(float a    =  1.f,
                      float b    =  0.f,
                      float c    =  0.f,
                      float tmin = -1.f,
                      float tmax =  1.f);

    [[nodiscard]] Vec3  evaluate(float t)          const override;
    [[nodiscard]] Vec3  derivative(float t)         const override;
    [[nodiscard]] Vec3  second_derivative(float t)  const override;
    [[nodiscard]] Vec3  third_derivative(float t)   const override;
    [[nodiscard]] float curvature(float t)           const override;

    [[nodiscard]] float t_min() const override { return m_tmin; }
    [[nodiscard]] float t_max() const override { return m_tmax; }

    // Coefficient accessors (for UI and testing)
    [[nodiscard]] float a() const noexcept { return m_a; }
    [[nodiscard]] float b() const noexcept { return m_b; }
    [[nodiscard]] float c() const noexcept { return m_c; }

    /// Vertex of the parabola: x_v = -b/2a, y_v = c - b^2/4a
    [[nodiscard]] Vec2 vertex() const noexcept;

private:
    float m_a, m_b, m_c;
    float m_tmin, m_tmax;
};

// ── Hyperbola ─────────────────────────────────────────────────────────────────
// Standard hyperbola centred at (h, k), parameterised by hyperbolic functions:
//   Horizontal: x(t) = h +/- a*cosh(t),  y(t) = k + b*sinh(t)
//   Vertical:   x(t) = h + b*sinh(t),    y(t) = k +/- a*cosh(t)
//
// Canonical form: (x-h)^2/a^2 - (y-k)^2/b^2 = 1  (horizontal axis)
//
// two_branch_vertex_count() and tessellate_two_branch() emit both branches
// into a contiguous span separated by a degenerate sentinel vertex (NaN pos,
// alpha=0) that the renderer skips via a primitive restart workaround.

enum class HyperbolaAxis { Horizontal, Vertical };

class Hyperbola final : public IConic {
public:
    explicit Hyperbola(float         a          = 1.f,
                       float         b          = 1.f,
                       float         h          = 0.f,
                       float         k          = 0.f,
                       HyperbolaAxis axis       = HyperbolaAxis::Horizontal,
                       float         half_range = 2.f);

    // IConic interface — evaluates the positive (right/upper) branch
    [[nodiscard]] Vec3  evaluate(float t)          const override;
    [[nodiscard]] Vec3  derivative(float t)         const override;
    [[nodiscard]] Vec3  second_derivative(float t)  const override;
    [[nodiscard]] Vec3  third_derivative(float t)   const override;

    [[nodiscard]] float t_min() const override { return -m_half_range; }
    [[nodiscard]] float t_max() const override { return  m_half_range; }

    // Coefficient accessors
    [[nodiscard]] float         a()    const noexcept { return m_a; }
    [[nodiscard]] float         b()    const noexcept { return m_b; }
    [[nodiscard]] float         h()    const noexcept { return m_h; }
    [[nodiscard]] float         k()    const noexcept { return m_k; }
    [[nodiscard]] HyperbolaAxis axis() const noexcept { return m_axis; }

    // ── Two-branch tessellation ───────────────────────────────────────────────

    /// Total vertices needed for both branches: 2*(n+1) + 1 (sentinel).
    [[nodiscard]] u32 two_branch_vertex_count(u32 n) const noexcept {
        return 2u * (n + 1u) + 1u;
    }

    /// Write both branches into out. The sentinel vertex at index (n+1) has
    /// pos = NaN and color.a = 0.0, which the shader discards.
    /// The caller must supply out with capacity >= two_branch_vertex_count(n).
    void tessellate_two_branch(std::span<Vertex> out,
                               u32               n,
                               Vec4              color = { 1.f, 1.f, 1.f, 1.f }) const;

private:
    float         m_a, m_b, m_h, m_k;
    HyperbolaAxis m_axis;
    float         m_half_range;

    [[nodiscard]] Vec3 eval_branch(float t, bool positive) const noexcept;
};

} // namespace ndde::math
