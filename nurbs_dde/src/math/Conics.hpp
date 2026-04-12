#pragma once
// math/Conics.hpp
// Parametric curve interface and concrete conic sections + space curves.
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
    [[nodiscard]] virtual float torsion(float t)           const;
    [[nodiscard]]         Vec3  unit_tangent(float t)      const;
    [[nodiscard]]         Vec3  unit_normal(float t)       const;
    [[nodiscard]]         Vec3  unit_binormal(float t)     const;

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
enum class HyperbolaAxis { Horizontal, Vertical };

class Hyperbola final : public IConic {
public:
    explicit Hyperbola(float a = 1.f, float b = 1.f,
                       float h = 0.f, float k = 0.f,
                       HyperbolaAxis axis       = HyperbolaAxis::Horizontal,
                       float         half_range = 2.f);

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

    [[nodiscard]] Vec3 eval_branch(float t, bool positive) const noexcept;

private:
    float         m_a, m_b, m_h, m_k;
    HyperbolaAxis m_axis;
    float         m_half_range;
};

// ── Helix ─────────────────────────────────────────────────────────────────────
// A circular helix in R^3, the canonical space curve:
//   p(t) = (r·cos(t),  r·sin(t),  b·t)
//
// where r is the radius and b = pitch / (2π) controls how fast it climbs.
//
// Frenet–Serret frame (analytic, exact):
//   T(t) = p'(t) / |p'(t)|
//   N(t) = principal normal = T'(t) / |T'(t)|   (points toward axis)
//   B(t) = T(t) × N(t)                           (binormal)
//
// For a circular helix these are all constant in magnitude:
//   κ = r / (r² + b²)       (curvature — constant)
//   τ = b / (r² + b²)       (torsion — constant)
//   |p'| = √(r² + b²)       (speed — constant)
//
// Formal predicate:
//   Helix(r, b, t_min, t_max) defines a regular C∞ curve in R^3
//   with constant nonzero curvature and constant torsion iff r > 0, b ≠ 0.
//   Locus theorem: A curve has constant κ > 0 and constant τ iff it is a helix.

class Helix final : public IConic {
public:
    /// r     = radius of the circle in the xy-plane
    /// pitch = vertical rise per full revolution (2π in t)
    /// tmin, tmax = parameter range (in radians)
    explicit Helix(float r     = 1.f,
                   float pitch = 0.5f,
                   float tmin  = 0.f,
                   float tmax  = static_cast<float>(4.f * std::numbers::pi));

    [[nodiscard]] Vec3  evaluate(float t)          const override;
    [[nodiscard]] Vec3  derivative(float t)         const override;
    [[nodiscard]] Vec3  second_derivative(float t)  const override;
    [[nodiscard]] Vec3  third_derivative(float t)   const override;
    [[nodiscard]] float curvature(float t)           const override;
    [[nodiscard]] float torsion(float t)             const override;

    [[nodiscard]] float t_min() const override { return m_tmin; }
    [[nodiscard]] float t_max() const override { return m_tmax; }

    [[nodiscard]] float radius() const noexcept { return m_r; }
    [[nodiscard]] float pitch()  const noexcept { return m_pitch; }

    // b = pitch / (2π): the z-rise per radian
    [[nodiscard]] float b() const noexcept { return m_b; }

private:
    float m_r, m_pitch, m_b, m_tmin, m_tmax;
};

} // namespace ndde::math
