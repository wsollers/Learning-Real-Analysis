#pragma once
// app/SurfaceMeshCache.hpp
// Shared CPU-side tessellation cache for scene surface previews.

#include "math/GeometryTypes.hpp"
#include "math/Surfaces.hpp"

#include <vector>

namespace ndde {

enum class SurfaceFillColorMode : u8 {
    HeightCell,
    HeightVertex,
    GaussianCurvatureCell
};

struct SurfaceMeshOptions {
    u32 grid_lines = 64;
    float time = 0.f;
    float color_scale = 1.f;
    Vec4 wire_color{0.9f, 0.95f, 1.f, 0.45f};
    SurfaceFillColorMode fill_color_mode = SurfaceFillColorMode::HeightCell;
    bool build_contour = true;
};

class SurfaceMeshCache {
public:
    void mark_dirty() noexcept { m_dirty = true; }
    void clear();

    void rebuild_if_needed(const ndde::math::ISurface& surface,
                           const SurfaceMeshOptions& options);

    [[nodiscard]] const std::vector<Vertex>& fill_vertices() const noexcept { return m_fill; }
    [[nodiscard]] const std::vector<Vertex>& wire_vertices() const noexcept { return m_wire; }
    [[nodiscard]] const std::vector<Vertex>& contour_vertices() const noexcept { return m_contour; }

    [[nodiscard]] u32 fill_count() const noexcept { return m_fill_count; }
    [[nodiscard]] u32 wire_count() const noexcept { return m_wire_count; }
    [[nodiscard]] u32 contour_count() const noexcept { return m_contour_count; }

    [[nodiscard]] static Vec4 height_color(float z, float scale) noexcept;
    [[nodiscard]] static Vec4 curvature_color(float k, float scale) noexcept;

private:
    bool m_dirty = true;
    u32 m_cached_grid = 0;

    std::vector<Vertex> m_fill;
    std::vector<Vertex> m_wire;
    std::vector<Vertex> m_contour;
    u32 m_fill_count = 0;
    u32 m_wire_count = 0;
    u32 m_contour_count = 0;
};

} // namespace ndde
