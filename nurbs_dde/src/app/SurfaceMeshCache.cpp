#include "app/SurfaceMeshCache.hpp"

#include <algorithm>

namespace ndde {

void SurfaceMeshCache::clear() {
    m_fill.clear();
    m_wire.clear();
    m_contour.clear();
    m_fill_count = 0;
    m_wire_count = 0;
    m_contour_count = 0;
    m_cached_grid = 0;
    m_dirty = true;
}

Vec4 SurfaceMeshCache::height_color(float z, float scale) noexcept {
    const float t = std::clamp(z / (scale + 1e-9f), -1.f, 1.f);
    if (t >= 0.f)
        return {0.50f + t * 0.35f, 0.50f - t * 0.38f, 0.50f - t * 0.42f, 0.82f};
    const float s = -t;
    return {0.50f - s * 0.40f, 0.50f + s * 0.18f - s * s * 0.46f, 0.50f + s * 0.35f, 0.82f};
}

Vec4 SurfaceMeshCache::curvature_color(float k, float scale) noexcept {
    return height_color(k, scale);
}

void SurfaceMeshCache::rebuild_if_needed(const ndde::math::ISurface& surface,
                                         const SurfaceMeshOptions& options) {
    const u32 n = std::max(options.grid_lines, 2u);
    const bool grid_changed = n != m_cached_grid;
    if (!m_dirty && !grid_changed && !surface.is_time_varying()) return;

    const float time = options.time;
    const u32 fill_capacity = n * n * 6u;
    m_fill.resize(fill_capacity);

    const float u0 = surface.u_min(time);
    const float u1 = surface.u_max(time);
    const float v0 = surface.v_min(time);
    const float v1 = surface.v_max(time);
    const float du = (u1 - u0) / static_cast<float>(n);
    const float dv = (v1 - v0) / static_cast<float>(n);

    u32 idx = 0;
    for (u32 i = 0; i < n; ++i) {
        const float ua = u0 + static_cast<float>(i) * du;
        const float ub = ua + du;
        for (u32 j = 0; j < n; ++j) {
            const float va = v0 + static_cast<float>(j) * dv;
            const float vb = va + dv;

            const Vec3 p00 = surface.evaluate(ua, va, time);
            const Vec3 p10 = surface.evaluate(ub, va, time);
            const Vec3 p01 = surface.evaluate(ua, vb, time);
            const Vec3 p11 = surface.evaluate(ub, vb, time);

            const float uc = (ua + ub) * 0.5f;
            const float vc = (va + vb) * 0.5f;
            const Vec4 cell_color =
                options.fill_color_mode == SurfaceFillColorMode::GaussianCurvatureCell
                    ? curvature_color(surface.gaussian_curvature(uc, vc, time), options.color_scale)
                    : height_color(surface.evaluate(uc, vc, time).z, options.color_scale);

            const auto vertex_color = [&](float u, float v) {
                if (options.fill_color_mode == SurfaceFillColorMode::HeightVertex)
                    return height_color(surface.evaluate(u, v, time).z, options.color_scale);
                return cell_color;
            };

            const Vec4 c00 = vertex_color(ua, va);
            const Vec4 c10 = vertex_color(ub, va);
            const Vec4 c01 = vertex_color(ua, vb);
            const Vec4 c11 = vertex_color(ub, vb);

            m_fill[idx++] = {p00, c00}; m_fill[idx++] = {p10, c10};
            m_fill[idx++] = {p11, c11}; m_fill[idx++] = {p00, c00};
            m_fill[idx++] = {p11, c11}; m_fill[idx++] = {p01, c01};
        }
    }
    m_fill_count = idx;

    const u32 wire_capacity = surface.wireframe_vertex_count(n, n);
    m_wire.resize(wire_capacity);
    surface.tessellate_wireframe({m_wire.data(), wire_capacity},
                                 n, n, time, options.wire_color);
    m_wire_count = wire_capacity;

    if (options.build_contour) {
        m_contour.resize(fill_capacity);
        idx = 0;
        for (u32 i = 0; i < n; ++i) {
            const float ua = u0 + static_cast<float>(i) * du;
            const float ub = ua + du;
            for (u32 j = 0; j < n; ++j) {
                const float va = v0 + static_cast<float>(j) * dv;
                const float vb = va + dv;
                const Vec3 p00{ua, va, 0.f};
                const Vec3 p10{ub, va, 0.f};
                const Vec3 p01{ua, vb, 0.f};
                const Vec3 p11{ub, vb, 0.f};
                const Vec4 c00 = height_color(surface.evaluate(ua, va, time).z, options.color_scale);
                const Vec4 c10 = height_color(surface.evaluate(ub, va, time).z, options.color_scale);
                const Vec4 c01 = height_color(surface.evaluate(ua, vb, time).z, options.color_scale);
                const Vec4 c11 = height_color(surface.evaluate(ub, vb, time).z, options.color_scale);
                m_contour[idx++] = {p00, c00}; m_contour[idx++] = {p10, c10};
                m_contour[idx++] = {p11, c11}; m_contour[idx++] = {p00, c00};
                m_contour[idx++] = {p11, c11}; m_contour[idx++] = {p01, c01};
            }
        }
        m_contour_count = idx;
    } else {
        m_contour.clear();
        m_contour_count = 0;
    }

    m_cached_grid = n;
    m_dirty = false;
}

} // namespace ndde
