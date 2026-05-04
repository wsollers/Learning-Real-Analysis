#pragma once

#include "engine/RenderService.hpp"
#include "math/GeometryTypes.hpp"
#include "memory/MemoryService.hpp"

#include <algorithm>
#include <cmath>
#include <span>

namespace ndde {

inline void add_curve2d_line(memory::FrameVector<Vertex>& out, Vec2 a, Vec2 b, Vec4 color) {
    out.push_back(Vertex{.pos = {a.x, a.y, 0.f}, .color = color});
    out.push_back(Vertex{.pos = {b.x, b.y, 0.f}, .color = color});
}

inline memory::FrameVector<Vertex> build_curve2d_frenet_hover_overlay(std::span<const Vec2> curve,
                                                                       Vec2 hover,
                                                                       const RenderViewDomain& domain,
                                                                       bool show_frenet,
                                                                       bool show_osculating_circle,
                                                                       memory::MemoryService* memory_service) {
    memory::FrameVector<Vertex> out = memory_service ? memory_service->frame().make_vector<Vertex>()
                                                     : memory::FrameVector<Vertex>{};
    if (curve.size() < 4u || (!show_frenet && !show_osculating_circle))
        return out;

    const float span_u = std::max(domain.u_max - domain.u_min, 0.01f);
    const float span_v = std::max(domain.v_max - domain.v_min, 0.01f);
    const float diagonal = std::sqrt(span_u * span_u + span_v * span_v);
    const float snap_radius = diagonal * 0.055f;

    std::size_t best = 1u;
    float best_d2 = snap_radius * snap_radius;
    for (std::size_t i = 1u; i + 1u < curve.size(); ++i) {
        const Vec2 d = curve[i] - hover;
        const float d2 = d.x * d.x + d.y * d.y;
        if (d2 < best_d2) {
            best_d2 = d2;
            best = i;
        }
    }
    if (best_d2 >= snap_radius * snap_radius)
        return out;

    const Vec2 p0 = curve[best - 1u];
    const Vec2 p = curve[best];
    const Vec2 p2 = curve[best + 1u];
    Vec2 tangent = p2 - p0;
    const float tangent_len = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
    if (tangent_len < 1.0e-6f)
        return out;
    tangent /= tangent_len;
    Vec2 normal{-tangent.y, tangent.x};

    const Vec2 d1 = (p2 - p0) * 0.5f;
    const Vec2 d2 = p2 - 2.f * p + p0;
    const float cross = d1.x * d2.y - d1.y * d2.x;
    const float speed2 = d1.x * d1.x + d1.y * d1.y;
    const float speed = std::sqrt(std::max(speed2, 0.f));
    const float kappa = speed > 1.0e-6f ? cross / (speed * speed * speed) : 0.f;
    if (kappa < 0.f)
        normal = -normal;

    const float axis_len = diagonal * 0.035f;
    if (show_frenet) {
        add_curve2d_line(out, p, p + tangent * axis_len, {1.f, 0.48f, 0.12f, 0.98f});
        add_curve2d_line(out, p, p + normal * axis_len, {0.1f, 1.f, 0.45f, 0.98f});
    }

    const float abs_kappa = std::abs(kappa);
    if (show_osculating_circle && abs_kappa > 1.0e-5f) {
        constexpr u32 segments = 56u;
        constexpr float two_pi = 6.28318530717958647692f;
        const float radius = std::min(1.f / abs_kappa, diagonal * 0.22f);
        const Vec2 center = p + normal * radius;
        const Vec4 color{1.f, 0.92f, 0.18f, 0.74f};
        for (u32 i = 0; i < segments; ++i) {
            const float a0 = two_pi * static_cast<float>(i) / static_cast<float>(segments);
            const float a1 = two_pi * static_cast<float>(i + 1u) / static_cast<float>(segments);
            const Vec2 q0{center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius};
            const Vec2 q1{center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius};
            add_curve2d_line(out, q0, q1, color);
        }
    }
    return out;
}

} // namespace ndde
