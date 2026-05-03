#pragma once
// app/ContourWindowRenderer.hpp
// Shared submission path for the second-window contour map.

#include "app/ParticleSystem.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "engine/EngineAPI.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace ndde {

struct ContourWindowOptions {
    float extent = 4.f;
    bool draw_wire = false;
    Vec4 wire_color{0.95f, 0.95f, 1.f, 0.32f};
    float trail_alpha_floor = 0.70f;
    bool draw_heads = false;
    float head_radius = 0.055f;
};

inline void submit_contour_window(EngineAPI& api,
                                  const SurfaceMeshCache& mesh,
                                  const ParticleSystem& particles,
                                  const ContourWindowOptions& options)
{
    const Vec2 sz = api.viewport_size2();
    if (sz.x <= 0.f || sz.y <= 0.f || mesh.contour_count() == 0) return;

    const float aspect = sz.x / std::max(sz.y, 1.f);
    const Mat4 mvp = aspect >= 1.f
        ? glm::ortho(-options.extent * aspect, options.extent * aspect, -options.extent, options.extent, -1.f, 1.f)
        : glm::ortho(-options.extent, options.extent, -options.extent / aspect, options.extent / aspect, -1.f, 1.f);

    auto fill = api.acquire(mesh.contour_count());
    for (u32 i = 0; i < mesh.contour_count(); ++i) {
        fill.vertices()[i] = mesh.contour_vertices()[i];
        fill.vertices()[i].pos.z = 0.f;
    }
    api.submit_to(RenderTarget::Contour2D, fill, Topology::TriangleList, DrawMode::VertexColor, {1,1,1,1}, mvp);

    if (options.draw_wire && mesh.wire_count() > 0) {
        auto wire = api.acquire(mesh.wire_count());
        for (u32 i = 0; i < mesh.wire_count(); ++i) {
            wire.vertices()[i] = mesh.wire_vertices()[i];
            wire.vertices()[i].pos.z = 0.f;
            wire.vertices()[i].color = options.wire_color;
        }
        api.submit_to(RenderTarget::Contour2D, wire, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp);
    }

    for (const Particle& particle : particles.particles()) {
        const u32 n = particle.trail_vertex_count();
        if (n < 2) continue;

        auto trail = api.acquire(n);
        particle.tessellate_trail({trail.vertices(), n});
        for (u32 i = 0; i < n; ++i) {
            trail.vertices()[i].pos.z = 0.f;
            trail.vertices()[i].color.a = std::max(trail.vertices()[i].color.a, options.trail_alpha_floor);
        }
        api.submit_to(RenderTarget::Contour2D, trail, Topology::LineStrip, DrawMode::VertexColor, {1,1,1,1}, mvp);

        if (options.draw_heads) {
            const glm::vec2 uv = particle.head_uv();
            auto dot = api.acquire(4);
            const Vec4 col = particle.head_colour();
            const float r = options.head_radius;
            dot.vertices()[0] = {{uv.x - r, uv.y, 0.f}, col};
            dot.vertices()[1] = {{uv.x + r, uv.y, 0.f}, col};
            dot.vertices()[2] = {{uv.x, uv.y - r, 0.f}, col};
            dot.vertices()[3] = {{uv.x, uv.y + r, 0.f}, col};
            api.submit_to(RenderTarget::Contour2D, dot, Topology::LineList, DrawMode::VertexColor, {1,1,1,1}, mvp);
        }
    }
}

} // namespace ndde
