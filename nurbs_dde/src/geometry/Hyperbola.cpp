// geometry/Hyperbola.cpp
#include "Hyperbola.hpp"
#include <cmath>
#include <stdexcept>

namespace ndde::geometry {

Hyperbola::Hyperbola(float a, float b, float h, float k,
                     HyperbolaAxis axis, float half_range)
    : m_a(a), m_b(b), m_h(h), m_k(k)
    , m_axis(axis), m_half_range(half_range)
{
    if (a <= 0.f || b <= 0.f)
        throw std::invalid_argument("Hyperbola: a and b must be positive");
    if (half_range <= 0.f)
        throw std::invalid_argument("Hyperbola: half_range must be positive");
}

// evaluate() returns the positive (right/upper) branch — used by base Curve::tessellate
Vec3 Hyperbola::evaluate(float t) const {
    return eval_branch(t, true);
}

Vec3 Hyperbola::eval_branch(float t, bool pos) const {
    const float sign = pos ? 1.f : -1.f;
    if (m_axis == HyperbolaAxis::Horizontal) {
        // x = h ± a·cosh(t),  y = k + b·sinh(t)
        return Vec3{ m_h + sign * m_a * std::cosh(t),
                     m_k + m_b * std::sinh(t),
                     0.f };
    } else {
        // y = k ± a·cosh(t),  x = h + b·sinh(t)
        return Vec3{ m_h + m_b * std::sinh(t),
                     m_k + sign * m_a * std::cosh(t),
                     0.f };
    }
}

std::vector<math::Vec4> Hyperbola::two_branch_buffer(std::uint32_t n) const {
    if (n == 0) throw std::invalid_argument("two_branch_buffer: n must be > 0");

    std::vector<math::Vec4> verts;
    verts.reserve(2 * (n + 1) + 1);

    const float t0   = -m_half_range;
    const float t1   =  m_half_range;
    const float step = (t1 - t0) / static_cast<float>(n);

    // Positive branch
    for (std::uint32_t i = 0; i <= n; ++i) {
        Vec3 p = eval_branch(t0 + static_cast<float>(i) * step, true);
        verts.emplace_back(p.x, p.y, p.z, 1.f);
    }

    // Sentinel: w=0 signals primitive restart to the renderer
    verts.emplace_back(0.f, 0.f, 0.f, 0.f);

    // Negative branch (reversed so it draws cleanly left-to-right / bottom-to-top)
    for (std::uint32_t i = n; ; --i) {
        Vec3 p = eval_branch(t0 + static_cast<float>(i) * step, false);
        verts.emplace_back(p.x, p.y, p.z, 1.f);
        if (i == 0) break;
    }

    return verts;
}

} // namespace ndde::geometry
