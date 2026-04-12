#pragma once
// math/Axes.hpp
// Generates vertex data for coordinate axes and grid lines.
// build_axes() and build_grid() write into caller-supplied spans so the
// math layer never allocates GPU memory.

#include "math/Types.hpp"
#include <span>
#include <cmath>

namespace ndde::math {

namespace colors {
    inline constexpr Vec4 X_AXIS    { 1.00f, 0.22f, 0.22f, 1.00f };
    inline constexpr Vec4 Y_AXIS    { 0.22f, 1.00f, 0.22f, 1.00f };
    inline constexpr Vec4 Z_AXIS    { 0.22f, 0.22f, 1.00f, 1.00f };
    inline constexpr Vec4 GRID_MAJOR{ 0.30f, 0.30f, 0.30f, 1.00f };
    inline constexpr Vec4 GRID_MINOR{ 0.12f, 0.12f, 0.12f, 1.00f };
} // namespace colors

struct AxesConfig {
    float extent     = 2.0f;
    float major_step = 1.0f;
    float minor_step = 0.2f;
    bool  is_3d      = false;
};

[[nodiscard]] u32 grid_vertex_count(const AxesConfig& cfg) noexcept;
[[nodiscard]] u32 axes_vertex_count(const AxesConfig& cfg) noexcept;

void build_grid(std::span<Vertex> out, const AxesConfig& cfg);
void build_axes(std::span<Vertex> out, const AxesConfig& cfg);

} // namespace ndde::math
