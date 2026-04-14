// math/Surfaces.cpp
#include "math/Surfaces.hpp"
#include <glm/glm.hpp>
#include <cmath>
#include <stdexcept>
#include <numbers>

namespace ndde::math {

// ── ISurface base ─────────────────────────────────────────────────────────────

Vec3 ISurface::du(float u, float v) const {
    constexpr float h = 1e-4f;
    return (evaluate(u + h, v) - evaluate(u - h, v)) / (2.f * h);
}

Vec3 ISurface::dv(float u, float v) const {
    constexpr float h = 1e-4f;
    return (evaluate(u, v + h) - evaluate(u, v - h)) / (2.f * h);
}

Vec3 ISurface::unit_normal(float u, float v) const {
    const Vec3 cross = glm::cross(du(u, v), dv(u, v));
    const float len  = glm::length(cross);
    return (len > 1e-8f) ? cross / len : Vec3{0.f, 0.f, 1.f};
}

float ISurface::gaussian_curvature(float u, float v) const {
    // First fundamental form: E, F, G
    const Vec3 fu = du(u, v);
    const Vec3 fv = dv(u, v);
    const float E = glm::dot(fu, fu);
    const float F = glm::dot(fu, fv);
    const float G = glm::dot(fv, fv);

    // Second fundamental form via finite-difference second partials
    constexpr float h = 1e-3f;
    const Vec3 fuu = (evaluate(u+h,v) - 2.f*evaluate(u,v) + evaluate(u-h,v)) / (h*h);
    const Vec3 fuv = (evaluate(u+h,v+h) - evaluate(u+h,v-h)
                    - evaluate(u-h,v+h) + evaluate(u-h,v-h)) / (4.f*h*h);
    const Vec3 fvv = (evaluate(u,v+h) - 2.f*evaluate(u,v) + evaluate(u,v-h)) / (h*h);
    const Vec3 n   = unit_normal(u, v);

    const float L = glm::dot(fuu, n);
    const float M = glm::dot(fuv, n);
    const float N = glm::dot(fvv, n);

    const float denom = E*G - F*F;
    return (std::abs(denom) < 1e-12f) ? 0.f : (L*N - M*M) / denom;
}

float ISurface::mean_curvature(float u, float v) const {
    const Vec3 fu = du(u, v);
    const Vec3 fv = dv(u, v);
    const float E = glm::dot(fu, fu);
    const float F = glm::dot(fu, fv);
    const float G = glm::dot(fv, fv);

    constexpr float h = 1e-3f;
    const Vec3 fuu = (evaluate(u+h,v) - 2.f*evaluate(u,v) + evaluate(u-h,v)) / (h*h);
    const Vec3 fuv = (evaluate(u+h,v+h) - evaluate(u+h,v-h)
                    - evaluate(u-h,v+h) + evaluate(u-h,v-h)) / (4.f*h*h);
    const Vec3 fvv = (evaluate(u,v+h) - 2.f*evaluate(u,v) + evaluate(u,v-h)) / (h*h);
    const Vec3 n   = unit_normal(u, v);

    const float L = glm::dot(fuu, n);
    const float M = glm::dot(fuv, n);
    const float N = glm::dot(fvv, n);

    const float denom = 2.f * (E*G - F*F);
    return (std::abs(denom) < 1e-12f) ? 0.f : (E*N + G*L - 2.f*F*M) / denom;
}

u32 ISurface::wireframe_vertex_count(u32 u_lines, u32 v_lines) const noexcept {
    // u_lines lines of constant-u (each has v_lines+1 edges → v_lines+1 verts in LineStrip,
    // but we emit LineList so 2 verts per segment → v_lines*2 verts per u-line)
    // v_lines lines of constant-v (same reasoning)
    // For clean rendering we emit pairs so Topology::LineList is used:
    //   u_lines * v_lines * 2   (v segments × 2 verts each) for constant-u lines
    //   v_lines * u_lines * 2   (u segments × 2 verts each) for constant-v lines
    // Total: 4 * u_lines * v_lines  ... but that's the segment count, not vertex count.
    // Actually:
    //   For each u-line (constant u): v_lines segments → v_lines * 2 verts
    //   There are u_lines+1 u-line positions.
    //   For each v-line (constant v): u_lines segments → u_lines * 2 verts
    //   There are v_lines+1 v-line positions.
    return (u_lines + 1u) * v_lines * 2u +
           (v_lines + 1u) * u_lines * 2u;
}

void ISurface::tessellate_wireframe(std::span<Vertex> out,
                                    u32 u_lines, u32 v_lines,
                                    Vec4 color) const
{
    const u32 needed = wireframe_vertex_count(u_lines, v_lines);
    if (out.size() < static_cast<std::size_t>(needed))
        throw std::invalid_argument("[ISurface::tessellate_wireframe] span too small");

    const float u0   = u_min(), u1 = u_max();
    const float v0   = v_min(), v1 = v_max();
    const float du_s = (u1 - u0) / static_cast<float>(u_lines);
    const float dv_s = (v1 - v0) / static_cast<float>(v_lines);

    u32 idx = 0;

    // Lines of constant u (vary v)
    for (u32 i = 0; i <= u_lines; ++i) {
        const float u = u0 + static_cast<float>(i) * du_s;
        for (u32 j = 0; j < v_lines; ++j) {
            const float va = v0 + static_cast<float>(j)   * dv_s;
            const float vb = v0 + static_cast<float>(j+1) * dv_s;
            out[idx++] = Vertex{ evaluate(u, va), color };
            out[idx++] = Vertex{ evaluate(u, vb), color };
        }
    }

    // Lines of constant v (vary u)
    for (u32 j = 0; j <= v_lines; ++j) {
        const float v = v0 + static_cast<float>(j) * dv_s;
        for (u32 i = 0; i < u_lines; ++i) {
            const float ua = u0 + static_cast<float>(i)   * du_s;
            const float ub = u0 + static_cast<float>(i+1) * du_s;
            out[idx++] = Vertex{ evaluate(ua, v), color };
            out[idx++] = Vertex{ evaluate(ub, v), color };
        }
    }
}

// ── Paraboloid ────────────────────────────────────────────────────────────────
//   p(u, v) = (u·cos v,  u·sin v,  a·u²)

Paraboloid::Paraboloid(float a, float u_max, float vmin, float vmax)
    : m_a(a), m_umax(u_max), m_vmin(vmin), m_vmax(vmax)
{
    if (a <= 0.f)    throw std::invalid_argument("[Paraboloid] a must be > 0");
    if (u_max <= 0.f) throw std::invalid_argument("[Paraboloid] u_max must be > 0");
}

Vec3 Paraboloid::evaluate(float u, float v) const {
    return Vec3{ u * std::cos(v), u * std::sin(v), m_a * u * u };
}

Vec3 Paraboloid::du(float u, float v) const {
    // ∂p/∂u = (cos v,  sin v,  2au)
    return Vec3{ std::cos(v), std::sin(v), 2.f * m_a * u };
}

Vec3 Paraboloid::dv(float u, float v) const {
    // ∂p/∂v = (-u·sin v,  u·cos v,  0)
    return Vec3{ -u * std::sin(v), u * std::cos(v), 0.f };
}

Vec3 Paraboloid::unit_normal(float u, float v) const {
    // N = (-2au·cos v, -2au·sin v, u) / (u·√(1 + 4a²u²))
    // At u = 0 this degenerates to (0,0,1).
    if (std::abs(u) < 1e-7f) return Vec3{ 0.f, 0.f, 1.f };
    const float denom = std::abs(u) * std::sqrt(1.f + 4.f*m_a*m_a*u*u);
    return Vec3{ -2.f*m_a*u*std::cos(v), -2.f*m_a*u*std::sin(v), u } / denom;
}

float Paraboloid::gaussian_curvature(float u, float /*v*/) const {
    // K = 4a² / (1 + 4a²u²)²
    const float base = 1.f + 4.f*m_a*m_a*u*u;
    return 4.f*m_a*m_a / (base * base);
}

float Paraboloid::mean_curvature(float u, float /*v*/) const {
    // H = a(2 + 4a²u²) / (1 + 4a²u²)^(3/2)
    const float s = 1.f + 4.f*m_a*m_a*u*u;
    return m_a * (2.f + 4.f*m_a*m_a*u*u) / std::pow(s, 1.5f);
}

float Paraboloid::kappa1(float u) const noexcept {
    // κ₁ = 2a / (1 + 4a²u²)^(3/2)   (in the meridional / u-direction)
    const float s = 1.f + 4.f*m_a*m_a*u*u;
    return 2.f*m_a / std::pow(s, 1.5f);
}

float Paraboloid::kappa2(float u) const noexcept {
    // κ₂ = 2a / √(1 + 4a²u²)         (in the latitudinal / v-direction)
    return 2.f*m_a / std::sqrt(1.f + 4.f*m_a*m_a*u*u);
}

} // namespace ndde::math
