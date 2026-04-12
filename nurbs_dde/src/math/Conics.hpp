#pragma once
// math/Conics.hpp
// Parametric curve interface and concrete conic sections.
// Zero-copy design: tessellate() writes directly into GPU-visible memory.

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

    [[nodiscard]] virtual Vec3  evaluate(float t)         const = 0;
    [[nodiscard]] virtual float t_min()                   const = 0;
    [[nodiscard]] virtual float t_max()                   const = 0;

    [[nodiscard]] virtual Vec3  derivative(float t)        const;
    [[nodiscard]] virtual Vec3  second_derivative(float t) const;
    [[nodiscard]] virtual Vec3  third_derivative(float t)  const;
    [[nodiscard]] virtual float curvature(float t)         const;
    [[nodiscard]]         Vec3  unit_tangent(float t)      const;

    void tessellate(std::span<Vertex> out, u32 n,
                    Vec4 color = { 1.f, 1.f, 1.f, 1.f }) const;

    [[nodiscard]] u32 vertex_count(u32 n) const noexcept { return n + 1; }
};

// ── Parabola ──────────────────────────────────────────────────────────────────
// p(t) = (t,  a*t^2 + b*t + c,  0)   t in [t_min, t_max]

class Parabola final : public IConic {
public:
    explicit Parabola(float a = 1.f, float b = 0.f, float c = 0.f,
                      float tmin = -1.f, float tmax = 1.f);

    [[nodiscard]] Vec3  evaluate(float t)          const override;
    [[nodiscard]] Vec3  derivative(float t)         const override;
    [[nodiscard]] Vec3  second_derivative(float t)  const override;
    [[nodiscard]] Vec3  third_derivative(float t)   const override;
    [[nodiscard]] float curvature(float t)           const override;

    [[nodiscard]] float t_min() const override { return m_tmin; }
    [[nodiscard]] float t_max() const override { return m_tmax; }

    [[nodiscard]] float a() const noexcept { return m_a; }
    [[nodiscard]] float b() const noexcept { return m_b; }
    [[nodiscard]] float c() const noexcept { return m_c; }
    [[nodiscard]] Vec2  vertex() const noexcept;

private:
    float m_a, m_b, m_c, m_tmin, m_tmax;
};

// ── Hyperbola ─────────────────────────────────────────────────────────────────
// Horizontal: x(t) = h +/- a*cosh(t),  y(t) = k + b*sinh(t)
// Vertical:   x(t) = h + b*sinh(t),    y(t) = k +/- a*cosh(t)
//
// tessellate_two_branch() emits both branches separated by a sentinel vertex
// (pos=NaN, alpha=0) that the fragment shader discards, breaking the strip.

enum class HyperbolaAxis { Horizontal, Vertical };

class Hyperbola final : public IConic {
public:
    explicit Hyperbola(float a = 1.f, float b = 1.f,
                       float h = 0.f, float k = 0.f,
                       HyperbolaAxis axis       = HyperbolaAxis::Horizontal,
                       float         half_range = 2.f);

    // evaluate() always returns the positive (right/upper) branch
    [[nodiscard]] Vec3  evaluate(float t)          const override;
    [[nodiscard]] Vec3  derivative(float t)         const override;
    [[nodiscard]] Vec3  second_derivative(float t)  const override;
    [[nodiscard]] Vec3  third_derivative(float t)   const override;

    [[nodiscard]] float t_min() const override { return -m_half_range; }
    [[nodiscard]] float t_max() const override { return  m_half_range; }

    [[nodiscard]] float         a()    const noexcept { return m_a; }
    [[nodiscard]] float         b()    const noexcept { return m_b; }
    [[nodiscard]] float         h()    const noexcept { return m_h; }
    [[nodiscard]] float         k()    const noexcept { return m_k; }
    [[nodiscard]] HyperbolaAxis axis() const noexcept { return m_axis; }

    [[nodiscard]] u32 two_branch_vertex_count(u32 n) const noexcept {
        return 2u * (n + 1u) + 1u;
    }

    void tessellate_two_branch(std::span<Vertex> out, u32 n,
                               Vec4 color = { 1.f, 1.f, 1.f, 1.f }) const;

    /// Public: evaluate a specific branch.
    /// positive=true → right/upper branch (same as evaluate())
    /// positive=false → left/lower branch
    [[nodiscard]] Vec3 eval_branch(float t, bool positive) const noexcept;

private:
    float         m_a, m_b, m_h, m_k;
    HyperbolaAxis m_axis;
    float         m_half_range;
};

} // namespace ndde::math
