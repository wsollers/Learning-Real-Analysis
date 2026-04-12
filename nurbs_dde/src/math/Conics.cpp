// math/Conics.cpp
#include "math/Conics.hpp"
#include <glm/glm.hpp>
#include <cmath>
#include <stdexcept>
#include <limits>

namespace ndde::math {

Vec3 IConic::derivative(float t) const {
    const float span = t_max() - t_min();
    const float hh   = span * 1e-4f;
    const float tl   = std::max(t - hh, t_min());
    const float tr   = std::min(t + hh, t_max());
    return (evaluate(tr) - evaluate(tl)) / (tr - tl);
}

Vec3 IConic::second_derivative(float t) const {
    const float span = t_max() - t_min();
    const float hh   = span * 1e-4f;
    const float tl   = std::max(t - hh, t_min());
    const float tr   = std::min(t + hh, t_max());
    const float tc   = t;
    return (evaluate(tr) - 2.f * evaluate(tc) + evaluate(tl)) / (hh * hh);
}

Vec3 IConic::third_derivative(float t) const {
    const float span = t_max() - t_min();
    const float hh   = span * 1e-3f;
    const float tl   = std::max(t - hh, t_min());
    const float tr   = std::min(t + hh, t_max());
    return (second_derivative(tr) - second_derivative(tl)) / (tr - tl);
}

float IConic::curvature(float t) const {
    const Vec3 d1  = derivative(t);
    const Vec3 d2  = second_derivative(t);
    const float len = glm::length(d1);
    if (len < 1e-8f) return 0.f;
    return glm::length(glm::cross(d1, d2)) / (len * len * len);
}

Vec3 IConic::unit_tangent(float t) const {
    const Vec3 d = derivative(t);
    const float len = glm::length(d);
    if (len < 1e-8f) return Vec3{1.f, 0.f, 0.f};
    return d / len;
}

void IConic::tessellate(std::span<Vertex> out, u32 n, Vec4 color) const {
    if (out.size() < static_cast<std::size_t>(n + 1u))
        throw std::invalid_argument("[IConic::tessellate] output span too small");

    const float t0   = t_min();
    const float step = (t_max() - t0) / static_cast<float>(n);

    for (u32 i = 0; i <= n; ++i) {
        const float t = t0 + static_cast<float>(i) * step;
        out[i] = Vertex{ evaluate(t), color };
    }
}

// ── Parabola ──────────────────────────────────────────────────────────────────

Parabola::Parabola(float a, float b, float c, float tmin, float tmax)
    : m_a(a), m_b(b), m_c(c), m_tmin(tmin), m_tmax(tmax)
{
    if (tmin >= tmax)
        throw std::invalid_argument("[Parabola] t_min must be strictly less than t_max");
}

Vec3 Parabola::evaluate(float t) const {
    return Vec3{ t, m_a * t * t + m_b * t + m_c, 0.f };
}

Vec3 Parabola::derivative(float t) const {
    return Vec3{ 1.f, 2.f * m_a * t + m_b, 0.f };
}

Vec3 Parabola::second_derivative(float /*t*/) const {
    return Vec3{ 0.f, 2.f * m_a, 0.f };
}

Vec3 Parabola::third_derivative(float /*t*/) const {
    return Vec3{ 0.f, 0.f, 0.f };
}

float Parabola::curvature(float t) const {
    const float dy_dt = 2.f * m_a * t + m_b;
    const float denom = std::pow(1.f + dy_dt * dy_dt, 1.5f);
    if (denom < 1e-8f) return 0.f;
    return std::abs(2.f * m_a) / denom;
}

Vec2 Parabola::vertex() const noexcept {
    if (std::abs(m_a) < 1e-8f) return Vec2{0.f, m_c};
    const float xv = -m_b / (2.f * m_a);
    return Vec2{ xv, m_a * xv * xv + m_b * xv + m_c };
}

// ── Hyperbola ─────────────────────────────────────────────────────────────────

Hyperbola::Hyperbola(float a, float b, float h, float k,
                     HyperbolaAxis axis, float half_range)
    : m_a(a), m_b(b), m_h(h), m_k(k), m_axis(axis), m_half_range(half_range)
{
    if (a <= 0.f || b <= 0.f)
        throw std::invalid_argument("[Hyperbola] a and b must be positive");
    if (half_range <= 0.f)
        throw std::invalid_argument("[Hyperbola] half_range must be positive");
}

Vec3 Hyperbola::eval_branch(float t, bool positive) const noexcept {
    const float sign = positive ? 1.f : -1.f;
    if (m_axis == HyperbolaAxis::Horizontal) {
        return Vec3{ m_h + sign * m_a * std::cosh(t),
                     m_k + m_b * std::sinh(t), 0.f };
    } else {
        return Vec3{ m_h + m_b * std::sinh(t),
                     m_k + sign * m_a * std::cosh(t), 0.f };
    }
}

Vec3 Hyperbola::evaluate(float t) const { return eval_branch(t, true); }

Vec3 Hyperbola::derivative(float t) const {
    if (m_axis == HyperbolaAxis::Horizontal)
        return Vec3{ m_a * std::sinh(t), m_b * std::cosh(t), 0.f };
    else
        return Vec3{ m_b * std::cosh(t), m_a * std::sinh(t), 0.f };
}

Vec3 Hyperbola::second_derivative(float t) const {
    if (m_axis == HyperbolaAxis::Horizontal)
        return Vec3{ m_a * std::cosh(t), m_b * std::sinh(t), 0.f };
    else
        return Vec3{ m_b * std::sinh(t), m_a * std::cosh(t), 0.f };
}

Vec3 Hyperbola::third_derivative(float t) const {
    return derivative(t);
}

void Hyperbola::tessellate_two_branch(std::span<Vertex> out, u32 n, Vec4 color) const {
    const u32 needed = two_branch_vertex_count(n);
    if (out.size() < static_cast<std::size_t>(needed))
        throw std::invalid_argument("[Hyperbola::tessellate_two_branch] output span too small");

    const float t0   = -m_half_range;
    const float step = (2.f * m_half_range) / static_cast<float>(n);

    for (u32 i = 0; i <= n; ++i)
        out[i] = Vertex{ eval_branch(t0 + static_cast<float>(i) * step, true), color };

    const float nan_val = std::numeric_limits<float>::quiet_NaN();
    out[n + 1u] = Vertex{ Vec3{ nan_val, nan_val, nan_val }, Vec4{ 0.f, 0.f, 0.f, 0.f } };

    const u32 base = n + 2u;
    for (u32 i = 0; i <= n; ++i) {
        const u32 src = n - i;
        out[base + i] = Vertex{ eval_branch(t0 + static_cast<float>(src) * step, false), color };
    }
}

} // namespace ndde::math
