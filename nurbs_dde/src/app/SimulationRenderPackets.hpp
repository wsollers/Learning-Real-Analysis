#pragma once
// app/SimulationRenderPackets.hpp
// Shared renderer-neutral packet emission for ISimulation surface prototypes.

#include "app/ParticleSystem.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "engine/RenderService.hpp"
#include "math/Surfaces.hpp"
#include "numeric/ops.hpp"

#include <algorithm>
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <limits>
#include <span>
#include <optional>
#include <vector>

namespace ndde {

inline RenderViewDomain surface_domain(const math::ISurface& surface, float time = 0.f) {
    const float u0 = surface.u_min(time);
    const float u1 = surface.u_max(time);
    const float v0 = surface.v_min(time);
    const float v1 = surface.v_max(time);

    const Vec3 center = surface.evaluate((u0 + u1) * 0.5f, (v0 + v1) * 0.5f, time);
    return RenderViewDomain{
        .u_min = u0,
        .u_max = u1,
        .v_min = v0,
        .v_max = v1,
        .z_min = center.z - 2.f,
        .z_max = center.z + 2.f
    };
}

inline Mat4 surface_main_mvp(const math::ISurface& surface,
                             const RenderViewDescriptor* descriptor,
                             float time = 0.f) {
    const RenderViewDomain domain = surface_domain(surface, time);
    const Vec3 center{
        descriptor ? descriptor->camera.target.x : (domain.u_min + domain.u_max) * 0.5f,
        descriptor ? descriptor->camera.target.y : (domain.v_min + domain.v_max) * 0.5f,
        descriptor ? descriptor->camera.target.z : surface.evaluate((domain.u_min + domain.u_max) * 0.5f,
                                                                    (domain.v_min + domain.v_max) * 0.5f,
                                                                    time).z
    };

    const float span_u = std::max(domain.u_max - domain.u_min, 0.01f);
    const float span_v = std::max(domain.v_max - domain.v_min, 0.01f);
    const float radius = std::max(span_u, span_v) * 0.62f;
    const CameraState camera = descriptor ? descriptor->camera : CameraState{};
    const float zoom = std::max(camera.zoom, 0.05f);
    const float dist = std::max(radius * 2.35f / zoom, 4.f);
    const float yaw = camera.yaw;
    const float pitch = camera.pitch;

    const Vec3 eye{
        center.x + dist * ops::cos(pitch) * ops::cos(yaw),
        center.y + dist * ops::cos(pitch) * ops::sin(yaw),
        center.z + dist * ops::sin(pitch)
    };

    const float aspect = descriptor ? std::max(descriptor->viewport_aspect, 0.1f) : 16.f / 9.f;
    const Mat4 proj = glm::perspective(glm::radians(43.f), aspect, 0.05f, 400.f);
    const Mat4 view = glm::lookAt(eye, center, Vec3{0.f, 0.f, 1.f});
    return proj * view;
}

inline Mat4 surface_alternate_mvp(const math::ISurface& surface, float time = 0.f) {
    const float u0 = surface.u_min(time);
    const float u1 = surface.u_max(time);
    const float v0 = surface.v_min(time);
    const float v1 = surface.v_max(time);
    const float du = std::max(u1 - u0, 0.01f);
    const float dv = std::max(v1 - v0, 0.01f);
    const float pad_u = du * 0.04f;
    const float pad_v = dv * 0.04f;
    return glm::ortho(u0 - pad_u, u1 + pad_u, v0 - pad_v, v1 + pad_v, -10.f, 10.f);
}

inline std::optional<Vec2> pick_surface_uv_by_ray(const math::ISurface& surface,
                                                  const RenderViewDescriptor* descriptor,
                                                  Vec2 screen_ndc,
                                                  float time = 0.f) {
    const Mat4 inv = glm::inverse(surface_main_mvp(surface, descriptor, time));
    const glm::vec4 near4 = inv * glm::vec4(screen_ndc.x, screen_ndc.y, 0.f, 1.f);
    const glm::vec4 far4 = inv * glm::vec4(screen_ndc.x, screen_ndc.y, 1.f, 1.f);
    if (std::abs(near4.w) < 1e-6f || std::abs(far4.w) < 1e-6f)
        return std::nullopt;

    const Vec3 origin = Vec3{near4.x, near4.y, near4.z} / near4.w;
    const Vec3 far_point = Vec3{far4.x, far4.y, far4.z} / far4.w;
    const Vec3 dir = glm::normalize(far_point - origin);

    const float u0 = surface.u_min(time);
    const float u1 = surface.u_max(time);
    const float v0 = surface.v_min(time);
    const float v1 = surface.v_max(time);
    auto in_domain = [&](float t) {
        const Vec3 p = origin + dir * t;
        return p.x >= u0 && p.x <= u1 && p.y >= v0 && p.y <= v1;
    };
    auto f = [&](float t) {
        const Vec3 p = origin + dir * t;
        return p.z - surface.evaluate(p.x, p.y, time).z;
    };

    constexpr int samples = 256;
    constexpr float t_min = 0.05f;
    constexpr float t_max = 400.f;
    float best_t = t_min;
    float best_abs = std::numeric_limits<float>::max();
    float prev_t = t_min;
    float prev_f = f(prev_t);
    bool prev_valid = in_domain(prev_t);

    for (int i = 1; i <= samples; ++i) {
        const float t = t_min + (t_max - t_min) * static_cast<float>(i) / static_cast<float>(samples);
        if (!in_domain(t)) continue;
        const float ft = f(t);
        const float aft = std::abs(ft);
        if (aft < best_abs) {
            best_abs = aft;
            best_t = t;
        }
        if (prev_valid && ((prev_f <= 0.f && ft >= 0.f) || (prev_f >= 0.f && ft <= 0.f))) {
            float lo = prev_t;
            float hi = t;
            float flo = prev_f;
            for (int it = 0; it < 32; ++it) {
                const float mid = 0.5f * (lo + hi);
                const float fm = f(mid);
                if ((flo <= 0.f && fm >= 0.f) || (flo >= 0.f && fm <= 0.f)) {
                    hi = mid;
                } else {
                    lo = mid;
                    flo = fm;
                }
            }
            const Vec3 p = origin + dir * (0.5f * (lo + hi));
            return Vec2{std::clamp(p.x, u0, u1), std::clamp(p.y, v0, v1)};
        }
        prev_t = t;
        prev_f = ft;
        prev_valid = true;
    }

    if (best_abs < 0.35f) {
        const Vec3 p = origin + dir * best_t;
        return Vec2{std::clamp(p.x, u0, u1), std::clamp(p.y, v0, v1)};
    }
    return std::nullopt;
}

inline void add_iso_segment(std::vector<Vertex>& out,
                            const Vec2& a, float za,
                            const Vec2& b, float zb,
                            float level,
                            Vec4 color) {
    const float denom = zb - za;
    const float t = std::abs(denom) > 1e-6f ? std::clamp((level - za) / denom, 0.f, 1.f) : 0.5f;
    out.push_back({{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, 0.f}, color});
}

inline std::vector<Vertex> build_level_curve_vertices(const math::ISurface& surface,
                                                      u32 grid,
                                                      float time,
                                                      u32 levels,
                                                      Vec4 color) {
    const u32 n = std::max(grid, 8u);
    const float u0 = surface.u_min(time);
    const float u1 = surface.u_max(time);
    const float v0 = surface.v_min(time);
    const float v1 = surface.v_max(time);
    const float du = (u1 - u0) / static_cast<float>(n);
    const float dv = (v1 - v0) / static_cast<float>(n);

    float zmin = std::numeric_limits<float>::max();
    float zmax = -std::numeric_limits<float>::max();
    std::vector<float> z((n + 1u) * (n + 1u));
    const auto idx = [n](u32 i, u32 j) { return i * (n + 1u) + j; };
    for (u32 i = 0; i <= n; ++i) {
        for (u32 j = 0; j <= n; ++j) {
            const float zz = surface.evaluate(u0 + du * static_cast<float>(i),
                                              v0 + dv * static_cast<float>(j), time).z;
            z[idx(i, j)] = zz;
            zmin = std::min(zmin, zz);
            zmax = std::max(zmax, zz);
        }
    }

    std::vector<Vertex> out;
    if (zmax <= zmin + 1e-6f) return out;
    for (u32 l = 1; l <= levels; ++l) {
        const float level = zmin + (zmax - zmin) * static_cast<float>(l) / static_cast<float>(levels + 1u);
        for (u32 i = 0; i < n; ++i) {
            for (u32 j = 0; j < n; ++j) {
                const Vec2 p00{u0 + du * static_cast<float>(i), v0 + dv * static_cast<float>(j)};
                const Vec2 p10{p00.x + du, p00.y};
                const Vec2 p01{p00.x, p00.y + dv};
                const Vec2 p11{p00.x + du, p00.y + dv};
                const float z00 = z[idx(i, j)];
                const float z10 = z[idx(i + 1u, j)];
                const float z01 = z[idx(i, j + 1u)];
                const float z11 = z[idx(i + 1u, j + 1u)];
                std::array<Vertex, 4> crossings{};
                u32 c = 0;
                auto edge = [&](const Vec2& a, float za, const Vec2& b, float zb) {
                    if ((za < level && zb >= level) || (za >= level && zb < level))
                        add_iso_segment(out, a, za, b, zb, level, color), ++c;
                };
                const std::size_t before = out.size();
                edge(p00, z00, p10, z10);
                edge(p10, z10, p11, z11);
                edge(p11, z11, p01, z01);
                edge(p01, z01, p00, z00);
                if (c == 2) {
                    // Already appended as a two-vertex segment.
                } else if (c != 0) {
                    out.resize(before);
                }
            }
        }
    }
    return out;
}

inline std::vector<Vertex> build_vector_field_vertices(const math::ISurface& surface,
                                                       u32 samples,
                                                       float time,
                                                       bool downhill,
                                                       Vec4 color) {
    const u32 n = std::max(samples, 4u);
    const float u0 = surface.u_min(time);
    const float u1 = surface.u_max(time);
    const float v0 = surface.v_min(time);
    const float v1 = surface.v_max(time);
    const float du = (u1 - u0) / static_cast<float>(n);
    const float dv = (v1 - v0) / static_cast<float>(n);
    const float len = std::min(du, dv) * 0.38f;
    std::vector<Vertex> out;
    out.reserve(n * n * 2u);
    for (u32 i = 0; i < n; ++i) {
        for (u32 j = 0; j < n; ++j) {
            const float u = u0 + du * (static_cast<float>(i) + 0.5f);
            const float v = v0 + dv * (static_cast<float>(j) + 0.5f);
            Vec2 g{surface.du(u, v, time).z, surface.dv(u, v, time).z};
            if (downhill) g = -g;
            if (glm::length(g) < 1e-5f) continue;
            g = glm::normalize(g) * len;
            out.push_back({{u, v, 0.f}, color});
            out.push_back({{u + g.x, v + g.y, 0.f}, color});
        }
    }
    return out;
}

inline void submit_typed_alternate_view(RenderService& render,
                                        RenderViewId alternate_view,
                                        const math::ISurface& surface,
                                        const SurfaceMeshCache& mesh,
                                        const ParticleSystem& particles,
                                        SurfaceMeshOptions options,
                                        const Mat4& alternate_mvp) {
    const RenderViewDescriptor* descriptor = render.descriptor(alternate_view);
    const AlternateViewMode mode = descriptor ? descriptor->alternate_mode : AlternateViewMode::Contour;
    if (mode == AlternateViewMode::Contour && mesh.contour_count() > 0) {
        render.submit(alternate_view,
            std::span<const Vertex>(mesh.contour_vertices().data(), mesh.contour_count()),
            Topology::TriangleList, DrawMode::VertexColor, {1.f, 1.f, 1.f, 1.f}, alternate_mvp);
        return;
    }

    if (mode == AlternateViewMode::LevelCurves || mode == AlternateViewMode::Isoclines) {
        const bool isoclines = mode == AlternateViewMode::Isoclines;
        if (isoclines) {
            auto u_lines = build_level_curve_vertices(surface, options.grid_lines, options.time, 1u,
                {0.2f, 0.85f, 1.f, 0.82f});
            render.submit(alternate_view, u_lines, Topology::LineList, DrawMode::VertexColor,
                {1.f, 1.f, 1.f, 1.f}, alternate_mvp);
        } else {
            auto lines = build_level_curve_vertices(surface, options.grid_lines, options.time, 10u,
                {0.95f, 0.95f, 1.f, 0.72f});
            render.submit(alternate_view, lines, Topology::LineList, DrawMode::VertexColor,
                {1.f, 1.f, 1.f, 1.f}, alternate_mvp);
        }
        return;
    }

    if (mode == AlternateViewMode::VectorField || mode == AlternateViewMode::Flow) {
        if (mesh.contour_count() > 0) {
            render.submit(alternate_view,
                std::span<const Vertex>(mesh.contour_vertices().data(), mesh.contour_count()),
                Topology::TriangleList, DrawMode::VertexColor, {1.f, 1.f, 1.f, 0.45f}, alternate_mvp);
        }
        auto arrows = build_vector_field_vertices(surface, 18u, options.time, true,
            mode == AlternateViewMode::Flow ? Vec4{0.2f, 1.f, 0.65f, 0.82f} : Vec4{1.f, 0.85f, 0.25f, 0.82f});
        render.submit(alternate_view, arrows, Topology::LineList, DrawMode::VertexColor,
            {1.f, 1.f, 1.f, 1.f}, alternate_mvp);
        (void)particles;
    }
}

template <class Surface>
inline void submit_surface_sim_packets(RenderService& render,
                                       RenderViewId main_view,
                                       RenderViewId alternate_view,
                                       Surface& surface,
                                       SurfaceMeshCache& mesh,
                                       const ParticleSystem& particles,
                                       SurfaceMeshOptions options)
{
    mesh.rebuild_if_needed(surface, options);
    const RenderViewDomain domain = surface_domain(surface, options.time);
    render.set_view_domain(main_view, domain);
    render.set_view_domain(alternate_view, domain);

    const Mat4 main_mvp = surface_main_mvp(surface, render.descriptor(main_view), options.time);
    const Mat4 alternate_mvp = surface_alternate_mvp(surface, options.time);
    const RenderViewDescriptor* main_descriptor = render.descriptor(main_view);

    if (mesh.wire_count() > 0) {
        render.submit(main_view,
            std::span<const Vertex>(mesh.wire_vertices().data(), mesh.wire_count()),
            Topology::LineList, DrawMode::VertexColor, {1.f, 1.f, 1.f, 1.f}, main_mvp);
    }

    if (main_descriptor && main_descriptor->overlays.show_axes) {
        const float z = (domain.z_min + domain.z_max) * 0.5f;
        const Vec3 origin{0.f, 0.f, z};
        const std::array<Vertex, 6> axes{{
            {{domain.u_min, 0.f, z}, {1.f, 0.25f, 0.25f, 0.95f}},
            {{domain.u_max, 0.f, z}, {1.f, 0.25f, 0.25f, 0.95f}},
            {{0.f, domain.v_min, z}, {0.25f, 1.f, 0.35f, 0.95f}},
            {{0.f, domain.v_max, z}, {0.25f, 1.f, 0.35f, 0.95f}},
            {origin, {0.35f, 0.55f, 1.f, 0.95f}},
            {{origin.x, origin.y, z + std::max(domain.u_max - domain.u_min, domain.v_max - domain.v_min) * 0.20f},
                {0.35f, 0.55f, 1.f, 0.95f}},
        }};
        render.submit(main_view, axes, Topology::LineList, DrawMode::VertexColor,
            {1.f, 1.f, 1.f, 1.f}, main_mvp);
    }

    submit_typed_alternate_view(render, alternate_view, surface, mesh, particles, options, alternate_mvp);

    for (const auto& particle : particles.particles()) {
        const u32 count = particle.trail_vertex_count();
        if (count < 2u) continue;
        std::vector<Vertex> trail(count);
        particle.tessellate_trail(std::span<Vertex>(trail.data(), trail.size()));
        const std::span<const Vertex> trail_span(trail.data(), trail.size());
        render.submit(main_view, trail_span,
            Topology::LineStrip, DrawMode::VertexColor, {1.f, 1.f, 1.f, 1.f}, main_mvp);
        for (Vertex& vertex : trail)
            vertex.pos.z = 0.f;
        render.submit(alternate_view, trail_span,
            Topology::LineStrip, DrawMode::VertexColor, {1.f, 1.f, 1.f, 1.f}, alternate_mvp);
    }
}

} // namespace ndde
