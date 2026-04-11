#pragma once
// geometry/Hyperbola.hpp
// Standard hyperbola centred at (h, k):
//   Horizontal transverse axis:  (x-h)²/a² - (y-k)²/b² = 1
//   Vertical   transverse axis:  (y-k)²/a² - (x-h)²/b² = 1
//
// Parametric form (horizontal):
//   x(t) = h + a·cosh(t)   y(t) = k + b·sinh(t)   — right branch (t ∈ [-T, T])
//   x(t) = h - a·cosh(t)   y(t) = k + b·sinh(t)   — left  branch
//
// We tessellate both branches as separate LINE_STRIP segments
// joined in one vertex buffer with a primitive-restart sentinel (w=0).

#include "Curve.hpp"

namespace ndde::geometry {

enum class HyperbolaAxis { Horizontal, Vertical };

class Hyperbola final : public Curve {
public:
    /// @param a  semi-transverse axis  (> 0)
    /// @param b  semi-conjugate  axis  (> 0)
    /// @param h  centre x
    /// @param k  centre y
    /// @param axis  which axis is the transverse axis
    /// @param half_range  parameter range [-T, T] for each branch
    explicit Hyperbola(float a           = 1.f,
                       float b           = 1.f,
                       float h           = 0.f,
                       float k           = 0.f,
                       HyperbolaAxis axis= HyperbolaAxis::Horizontal,
                       float half_range  = 2.f);

    [[nodiscard]] Vec3  evaluate(float t)  const override;
    [[nodiscard]] float t_min()            const override { return -m_half_range; }
    [[nodiscard]] float t_max()            const override { return  m_half_range; }

    // Two-branch vertex buffer:
    //   right/upper branch forward, then w=0 sentinel, then left/lower branch
    [[nodiscard]] std::vector<math::Vec4> two_branch_buffer(std::uint32_t n) const;

    float a()    const { return m_a; }
    float b()    const { return m_b; }
    float h()    const { return m_h; }
    float k()    const { return m_k; }
    HyperbolaAxis axis() const { return m_axis; }

private:
    float         m_a, m_b, m_h, m_k;
    HyperbolaAxis m_axis;
    float         m_half_range;

    Vec3 eval_branch(float t, bool positive_branch) const;
};

} // namespace ndde::geometry
