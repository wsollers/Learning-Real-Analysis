#pragma once
// app/SimulationRenderPackets.hpp
// Shared renderer-neutral packet emission for ISimulation surface prototypes.

#include "app/ParticleSystem.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "engine/CameraService.hpp"
#include "engine/InteractionService.hpp"
#include "engine/RenderService.hpp"
#include "math/Surfaces.hpp"
#include "memory/Containers.hpp"
#include "memory/MemoryService.hpp"
#include "numeric/ops.hpp"

#include <algorithm>
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <limits>
#include <span>
#include <optional>

namespace ndde {

template <class T>
inline memory::FrameVector<T> make_frame_vector(memory::MemoryService* memory_service) {
    return memory_service ? memory_service->frame().make_vector<T>() : memory::FrameVector<T>{};
}

template <class T>
inline memory::FrameVector<T> make_frame_vector(memory::MemoryService* memory_service, std::size_t count) {
    return memory_service ? memory_service->frame().make_vector<T>(count) : memory::FrameVector<T>(count);
}

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

inline void add_iso_segment(memory::FrameVector<Vertex>& out,
                            const Vec2& a, float za,
                            const Vec2& b, float zb,
                            float level,
                            Vec4 color) {
    const float denom = zb - za;
    const float t = std::abs(denom) > 1e-6f ? std::clamp((level - za) / denom, 0.f, 1.f) : 0.5f;
    out.push_back({{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, 0.f}, color});
}

template <class ScalarSampler>
inline memory::FrameVector<Vertex> build_scalar_isoline_vertices(const math::ISurface& surface,
                                                         u32 grid,
                                                         float time,
                                                         std::span<const float> levels,
                                                         ScalarSampler&& sample,
                                                         Vec4 color,
                                                         memory::MemoryService* memory_service = nullptr) {
    const u32 n = std::max(grid, 8u);
    const float u0 = surface.u_min(time);
    const float u1 = surface.u_max(time);
    const float v0 = surface.v_min(time);
    const float v1 = surface.v_max(time);
    const float du = (u1 - u0) / static_cast<float>(n);
    const float dv = (v1 - v0) / static_cast<float>(n);

    memory::FrameVector<float> values = make_frame_vector<float>(memory_service, (n + 1u) * (n + 1u));
    const auto idx = [n](u32 i, u32 j) { return i * (n + 1u) + j; };
    for (u32 i = 0; i <= n; ++i) {
        for (u32 j = 0; j <= n; ++j) {
            values[idx(i, j)] = sample(u0 + du * static_cast<float>(i),
                                       v0 + dv * static_cast<float>(j));
        }
    }

    memory::FrameVector<Vertex> out = make_frame_vector<Vertex>(memory_service);
    for (const float level : levels) {
        for (u32 i = 0; i < n; ++i) {
            for (u32 j = 0; j < n; ++j) {
                const Vec2 p00{u0 + du * static_cast<float>(i), v0 + dv * static_cast<float>(j)};
                const Vec2 p10{p00.x + du, p00.y};
                const Vec2 p01{p00.x, p00.y + dv};
                const Vec2 p11{p00.x + du, p00.y + dv};
                const float z00 = values[idx(i, j)];
                const float z10 = values[idx(i + 1u, j)];
                const float z01 = values[idx(i, j + 1u)];
                const float z11 = values[idx(i + 1u, j + 1u)];
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
                if (c != 0 && c != 2)
                    out.resize(before);
            }
        }
    }
    return out;
}

inline memory::FrameVector<Vertex> build_level_curve_vertices(const math::ISurface& surface,
                                                      u32 grid,
                                                      float time,
                                                      u32 levels,
                                                      Vec4 color,
                                                      memory::MemoryService* memory_service = nullptr) {
    const u32 n = std::max(grid, 8u);
    const float u0 = surface.u_min(time);
    const float u1 = surface.u_max(time);
    const float v0 = surface.v_min(time);
    const float v1 = surface.v_max(time);
    const float du = (u1 - u0) / static_cast<float>(n);
    const float dv = (v1 - v0) / static_cast<float>(n);

    float zmin = std::numeric_limits<float>::max();
    float zmax = -std::numeric_limits<float>::max();
    for (u32 i = 0; i <= n; ++i) {
        for (u32 j = 0; j <= n; ++j) {
            const float z = surface.evaluate(u0 + du * static_cast<float>(i),
                                             v0 + dv * static_cast<float>(j), time).z;
            zmin = std::min(zmin, z);
            zmax = std::max(zmax, z);
        }
    }
    if (zmax <= zmin + 1e-6f) return make_frame_vector<Vertex>(memory_service);

    memory::FrameVector<float> scalar_levels = make_frame_vector<float>(memory_service);
    scalar_levels.reserve(levels);
    for (u32 l = 1; l <= levels; ++l)
        scalar_levels.push_back(zmin + (zmax - zmin) * static_cast<float>(l) / static_cast<float>(levels + 1u));

    return build_scalar_isoline_vertices(surface, grid, time, scalar_levels,
        [&](float u, float v) { return surface.evaluate(u, v, time).z; }, color, memory_service);
}

inline Vec2 surface_height_gradient(const math::ISurface& surface, float u, float v, float time) {
    return Vec2{surface.du(u, v, time).z, surface.dv(u, v, time).z};
}

inline memory::FrameVector<Vertex> build_isocline_vertices(const math::ISurface& surface,
                                                   u32 grid,
                                                   float time,
                                                   f32 direction_angle,
                                                   f32 target_slope,
                                                   f32 tolerance,
                                                   u32 bands,
                                                   Vec4 color,
                                                   memory::MemoryService* memory_service = nullptr) {
    const Vec2 dir{ops::cos(direction_angle), ops::sin(direction_angle)};
    const u32 count = std::max(bands, 1u);
    const float tol = std::max(tolerance, 0.001f);
    memory::FrameVector<float> levels = make_frame_vector<float>(memory_service);
    levels.reserve(count);
    if (count == 1u) {
        levels.push_back(target_slope);
    } else {
        for (u32 i = 0; i < count; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(count - 1u);
            levels.push_back(target_slope - tol + 2.f * tol * t);
        }
    }
    return build_scalar_isoline_vertices(surface, grid, time, levels,
        [&](float u, float v) {
            const Vec2 grad = surface_height_gradient(surface, u, v, time);
            return glm::dot(grad, dir);
        }, color, memory_service);
}

inline Vec2 surface_vector_at(const math::ISurface& surface,
                              float u,
                              float v,
                              float time,
                              VectorFieldMode mode) {
    const Vec2 grad = surface_height_gradient(surface, u, v, time);
    switch (mode) {
        case VectorFieldMode::Gradient:
            return grad;
        case VectorFieldMode::NegativeGradient:
            return -grad;
        case VectorFieldMode::LevelTangent:
            return Vec2{-grad.y, grad.x};
        case VectorFieldMode::ParticleVelocity:
            return -grad;
    }
    return -grad;
}

inline memory::FrameVector<Vertex> build_vector_field_vertices(const math::ISurface& surface,
                                                       u32 samples,
                                                       float time,
                                                       VectorFieldMode mode,
                                                       float scale,
                                                       Vec4 color,
                                                       memory::MemoryService* memory_service = nullptr) {
    const u32 n = std::max(samples, 4u);
    const float u0 = surface.u_min(time);
    const float u1 = surface.u_max(time);
    const float v0 = surface.v_min(time);
    const float v1 = surface.v_max(time);
    const float du = (u1 - u0) / static_cast<float>(n);
    const float dv = (v1 - v0) / static_cast<float>(n);
    const float len = std::min(du, dv) * 0.38f;
    memory::FrameVector<Vertex> out = make_frame_vector<Vertex>(memory_service);
    out.reserve(n * n * 2u);
    for (u32 i = 0; i < n; ++i) {
        for (u32 j = 0; j < n; ++j) {
            const float u = u0 + du * (static_cast<float>(i) + 0.5f);
            const float v = v0 + dv * (static_cast<float>(j) + 0.5f);
            Vec2 g = surface_vector_at(surface, u, v, time, mode);
            if (glm::length(g) < 1e-5f) continue;
            g = glm::normalize(g) * len * std::max(scale, 0.01f);
            out.push_back({{u, v, 0.f}, color});
            out.push_back({{u + g.x, v + g.y, 0.f}, color});
        }
    }
    return out;
}

inline memory::FrameVector<Vertex> build_particle_velocity_vertices(const ParticleSystem& particles,
                                                            float scale,
                                                            Vec4 color,
                                                            memory::MemoryService* memory_service = nullptr) {
    memory::FrameVector<Vertex> out = make_frame_vector<Vertex>(memory_service);
    for (const AnimatedCurve& particle : particles.particles()) {
        const u32 n = particle.trail_size();
        if (n < 2u) continue;
        const Vec3 prev = particle.trail_pt(n - 2u);
        const Vec3 head = particle.trail_pt(n - 1u);
        Vec2 velocity{head.x - prev.x, head.y - prev.y};
        if (glm::length(velocity) < 1e-5f) continue;
        velocity = glm::normalize(velocity) * std::max(scale, 0.01f) * 0.18f;
        out.push_back({{head.x, head.y, 0.f}, color});
        out.push_back({{head.x + velocity.x, head.y + velocity.y, 0.f}, color});
    }
    return out;
}

inline memory::FrameVector<Vertex> build_flow_vertices(const math::ISurface& surface,
                                               u32 seed_count,
                                               u32 steps,
                                               float step_size,
                                               float time,
                                               VectorFieldMode mode,
                                               Vec4 color,
                                               memory::MemoryService* memory_service = nullptr) {
    const u32 n = std::max(seed_count, 3u);
    const u32 step_count = std::max(steps, 1u);
    const float u0 = surface.u_min(time);
    const float u1 = surface.u_max(time);
    const float v0 = surface.v_min(time);
    const float v1 = surface.v_max(time);
    const float du = (u1 - u0) / static_cast<float>(n);
    const float dv = (v1 - v0) / static_cast<float>(n);
    const float step = std::min(u1 - u0, v1 - v0) * std::max(step_size, 0.001f);

    memory::FrameVector<Vertex> out = make_frame_vector<Vertex>(memory_service);
    out.reserve(n * n * step_count * 2u);
    for (u32 i = 0; i < n; ++i) {
        for (u32 j = 0; j < n; ++j) {
            Vec2 p{u0 + du * (static_cast<float>(i) + 0.5f),
                   v0 + dv * (static_cast<float>(j) + 0.5f)};
            for (u32 k = 0; k < step_count; ++k) {
                Vec2 direction = surface_vector_at(surface, p.x, p.y, time, mode);
                if (glm::length(direction) < 1e-5f) break;
                direction = glm::normalize(direction);
                const Vec2 next{p.x + direction.x * step, p.y + direction.y * step};
                if (next.x < u0 || next.x > u1 || next.y < v0 || next.y > v1)
                    break;
                out.push_back({{p.x, p.y, 0.f}, color});
                out.push_back({{next.x, next.y, 0.f}, color});
                p = next;
            }
        }
    }
    return out;
}

inline memory::FrameVector<Vertex> build_particle_frenet_overlay_vertices(const AnimatedCurve& particle,
                                                                  const RenderViewDomain& domain,
                                                                  u32 trail_idx,
                                                                  bool show_frenet,
                                                                  bool show_osculating_circle,
                                                                  memory::MemoryService* memory_service = nullptr) {
    memory::FrameVector<Vertex> out = make_frame_vector<Vertex>(memory_service);
    const u32 n = particle.trail_size();
    if (n < 4u || (!show_frenet && !show_osculating_circle)) return out;

    const u32 idx = std::clamp(trail_idx, 1u, n - 2u);
    const Vec3 p = particle.trail_pt(idx);
    const FrenetFrame fr = particle.frenet_at(idx);
    const float span = std::max(domain.u_max - domain.u_min, domain.v_max - domain.v_min);
    const float axis_len = std::max(span * 0.035f, 0.05f);

    if (show_frenet) {
        const auto add_axis = [&](Vec3 dir, Vec4 color) {
            if (glm::length(dir) < 1e-6f) return;
            dir = glm::normalize(dir) * axis_len;
            out.push_back({p, color});
            out.push_back({p + dir, color});
        };
        add_axis(fr.T, {1.f, 0.48f, 0.12f, 0.95f});
        add_axis(fr.N, {0.1f, 1.f, 0.45f, 0.95f});
        add_axis(fr.B, {0.35f, 0.62f, 1.f, 0.95f});
    }

    if (show_osculating_circle && fr.kappa > 1e-5f && glm::length(fr.T) > 1e-6f && glm::length(fr.N) > 1e-6f) {
        constexpr u32 segments = 48u;
        const float radius = std::min(1.f / fr.kappa, span * 0.18f);
        const Vec3 tangent = glm::normalize(fr.T);
        const Vec3 normal = glm::normalize(fr.N);
        const Vec3 center = p + normal * radius;
        const Vec4 color{1.f, 0.92f, 0.18f, 0.72f};
        for (u32 i = 0; i < segments; ++i) {
            const float a0 = ops::two_pi_v<float> * static_cast<float>(i) / static_cast<float>(segments);
            const float a1 = ops::two_pi_v<float> * static_cast<float>(i + 1u) / static_cast<float>(segments);
            const Vec3 p0 = center + (-normal * ops::cos(a0) + tangent * ops::sin(a0)) * radius;
            const Vec3 p1 = center + (-normal * ops::cos(a1) + tangent * ops::sin(a1)) * radius;
            out.push_back({p0, color});
            out.push_back({p1, color});
        }
    }
    return out;
}

inline memory::FrameVector<TrailPickSample> build_trail_pick_samples(const ParticleSystem& particles,
                                                                     memory::MemoryService* memory_service = nullptr) {
    memory::FrameVector<TrailPickSample> samples = make_frame_vector<TrailPickSample>(memory_service);
    const auto& list = particles.particles();
    for (std::size_t pi = 0; pi < list.size(); ++pi) {
        const AnimatedCurve& particle = list[pi];
        const u32 n = particle.trail_size();
        if (n < 4u) continue;
        for (u32 ti = 1u; ti + 1u < n; ++ti) {
            const FrenetFrame fr = particle.frenet_at(ti);
            samples.push_back(TrailPickSample{
                .particle_id = particle.id(),
                .particle_index = static_cast<u32>(pi),
                .trail_index = ti,
                .world = particle.trail_pt(ti),
                .curvature = fr.kappa,
                .torsion = fr.tau
            });
        }
    }
    return samples;
}

inline void submit_typed_alternate_view(RenderService& render,
                                        RenderViewId alternate_view,
                                        const math::ISurface& surface,
                                        const SurfaceMeshCache& mesh,
                                        const ParticleSystem& particles,
                                        SurfaceMeshOptions options,
                                        const Mat4& alternate_mvp,
                                        memory::MemoryService* memory_service = nullptr) {
    const RenderViewDescriptor* descriptor = render.descriptor(alternate_view);
    const AlternateViewMode mode = descriptor ? descriptor->alternate_mode : AlternateViewMode::Contour;
    if (mode == AlternateViewMode::Contour && mesh.contour_count() > 0) {
        render.submit(alternate_view,
            std::span<const Vertex>(mesh.contour_vertices().data(), mesh.contour_count()),
            Topology::TriangleList, DrawMode::VertexColor, {1.f, 1.f, 1.f, 1.f}, alternate_mvp);
        return;
    }

    if (mode == AlternateViewMode::LevelCurves || mode == AlternateViewMode::Isoclines) {
        if (mode == AlternateViewMode::Isoclines) {
            const AlternateViewSettings settings = descriptor ? descriptor->alternate : AlternateViewSettings{};
            auto u_lines = build_isocline_vertices(surface, options.grid_lines, options.time,
                settings.isocline_direction_angle, settings.isocline_target_slope,
                settings.isocline_tolerance, settings.isocline_bands,
                {0.2f, 0.85f, 1.f, 0.82f}, memory_service);
            render.submit(alternate_view, u_lines, Topology::LineList, DrawMode::VertexColor,
                {1.f, 1.f, 1.f, 1.f}, alternate_mvp);
        } else {
            auto lines = build_level_curve_vertices(surface, options.grid_lines, options.time, 10u,
                {0.95f, 0.95f, 1.f, 0.72f}, memory_service);
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
        const AlternateViewSettings settings = descriptor ? descriptor->alternate : AlternateViewSettings{};
        memory::FrameVector<Vertex> arrows = make_frame_vector<Vertex>(memory_service);
        if (mode == AlternateViewMode::Flow) {
            arrows = build_flow_vertices(surface, settings.flow_seed_count, settings.flow_steps,
                settings.flow_step_size, options.time, settings.vector_mode, {0.2f, 1.f, 0.65f, 0.82f}, memory_service);
        } else if (settings.vector_mode == VectorFieldMode::ParticleVelocity) {
            arrows = build_particle_velocity_vertices(particles, settings.vector_scale, {1.f, 0.55f, 0.2f, 0.9f},
                memory_service);
        } else {
            arrows = build_vector_field_vertices(surface, settings.vector_samples, options.time,
                settings.vector_mode, settings.vector_scale, {1.f, 0.85f, 0.25f, 0.82f}, memory_service);
        }
        render.submit(alternate_view, arrows, Topology::LineList, DrawMode::VertexColor,
            {1.f, 1.f, 1.f, 1.f}, alternate_mvp);
    }
}

template <class Surface>
inline void submit_surface_sim_packets(RenderService& render,
                                       RenderViewId main_view,
                                       RenderViewId alternate_view,
                                       Surface& surface,
                                       SurfaceMeshCache& mesh,
                                       const ParticleSystem& particles,
                                       SurfaceMeshOptions options,
                                       InteractionService* interaction = nullptr,
                                       memory::MemoryService* memory_service = nullptr,
                                       CameraService* camera_service = nullptr)
{
    mesh.bind_memory(memory_service);
    mesh.rebuild_if_needed(surface, options);
    const RenderViewDomain domain = surface_domain(surface, options.time);
    render.set_view_domain(main_view, domain);
    render.set_view_domain(alternate_view, domain);

    const Mat4 main_mvp = camera_service ? camera_service->perspective_mvp(main_view)
                                         : surface_main_mvp(surface, render.descriptor(main_view), options.time);
    const Mat4 alternate_mvp = camera_service ? camera_service->orthographic_mvp(alternate_view)
                                              : surface_alternate_mvp(surface, options.time);
    const RenderViewDescriptor* main_descriptor = render.descriptor(main_view);
    const memory::FrameVector<TrailPickSample> pick_samples =
        (interaction && main_descriptor) ? build_trail_pick_samples(particles, memory_service)
                                         : make_frame_vector<TrailPickSample>(memory_service);
    const ParticleTrailHit hover = (interaction && main_descriptor)
        ? interaction->resolve_particle_trail_hit(main_view, pick_samples, main_mvp, main_descriptor->viewport_size)
        : ParticleTrailHit{};

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

    submit_typed_alternate_view(render, alternate_view, surface, mesh, particles, options, alternate_mvp, memory_service);

    const auto& particle_list = particles.particles();
    for (std::size_t particle_index = 0; particle_index < particle_list.size(); ++particle_index) {
        const auto& particle = particle_list[particle_index];
        const u32 count = particle.trail_vertex_count();
        if (count < 2u) continue;
        memory::FrameVector<Vertex> trail = make_frame_vector<Vertex>(memory_service, count);
        particle.tessellate_trail(std::span<Vertex>(trail.data(), trail.size()));
        const std::span<const Vertex> trail_span(trail.data(), trail.size());
        render.submit(main_view, trail_span,
            Topology::LineStrip, DrawMode::VertexColor, {1.f, 1.f, 1.f, 1.f}, main_mvp);
        for (Vertex& vertex : trail)
            vertex.pos.z = 0.f;
        render.submit(alternate_view, trail_span,
            Topology::LineStrip, DrawMode::VertexColor, {1.f, 1.f, 1.f, 1.f}, alternate_mvp);

        if (main_descriptor && hover.hit && hover.particle_index == particle_index
            && (main_descriptor->overlays.show_hover_frenet
                || main_descriptor->overlays.show_osculating_circle)) {
            auto overlay = build_particle_frenet_overlay_vertices(particle, domain,
                hover.trail_index,
                main_descriptor->overlays.show_hover_frenet,
                main_descriptor->overlays.show_osculating_circle,
                memory_service);
            render.submit(main_view, overlay, Topology::LineList, DrawMode::VertexColor,
                {1.f, 1.f, 1.f, 1.f}, main_mvp);
        }
    }
}

} // namespace ndde
