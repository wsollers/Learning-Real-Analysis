// math/Axes.cpp
#include "math/Axes.hpp"
#include <cmath>
#include <stdexcept>

namespace ndde::math {

static u32 count_lines(float extent, float step) noexcept {
    const auto n = static_cast<u32>(std::floor(extent / step));
    return 2u * n;
}

u32 grid_vertex_count(const AxesConfig& cfg) noexcept {
    const u32 lines_per_axis = count_lines(cfg.extent, cfg.minor_step);
    const u32 z_extra = cfg.is_3d ? lines_per_axis * 2u : 0u;
    return (lines_per_axis * 2u + z_extra) * 2u;
}

u32 axes_vertex_count(const AxesConfig& cfg) noexcept {
    return cfg.is_3d ? 6u : 4u;
}

void build_grid(std::span<Vertex> out, const AxesConfig& cfg) {
    const u32 needed = grid_vertex_count(cfg);
    if (out.size() < static_cast<std::size_t>(needed))
        throw std::invalid_argument("[build_grid] output span too small");

    const float eps = cfg.minor_step * 0.01f;
    u32 idx = 0;

    auto push = [&](Vec3 a, Vec3 b, Vec4 col) {
        out[idx++] = Vertex{ a, col };
        out[idx++] = Vertex{ b, col };
    };

    auto grid_color = [&](float coord) -> Vec4 {
        const float rem = std::fmod(std::abs(coord) + eps * 0.5f, cfg.major_step);
        return (rem < eps) ? colors::GRID_MAJOR : colors::GRID_MINOR;
    };

    for (float x = cfg.minor_step; x <= cfg.extent + eps; x += cfg.minor_step) {
        const Vec4 col = grid_color(x);
        push({ x,  -cfg.extent, 0.f }, {  x, cfg.extent, 0.f }, col);
        push({ -x, -cfg.extent, 0.f }, { -x, cfg.extent, 0.f }, col);
    }

    for (float y = cfg.minor_step; y <= cfg.extent + eps; y += cfg.minor_step) {
        const Vec4 col = grid_color(y);
        push({ -cfg.extent,  y, 0.f }, { cfg.extent,  y, 0.f }, col);
        push({ -cfg.extent, -y, 0.f }, { cfg.extent, -y, 0.f }, col);
    }

    if (cfg.is_3d) {
        for (float x = cfg.minor_step; x <= cfg.extent + eps; x += cfg.minor_step) {
            const Vec4 col = grid_color(x);
            push({  x, 0.f, -cfg.extent }, {  x, 0.f, cfg.extent }, col);
            push({ -x, 0.f, -cfg.extent }, { -x, 0.f, cfg.extent }, col);
        }
        for (float y = cfg.minor_step; y <= cfg.extent + eps; y += cfg.minor_step) {
            const Vec4 col = grid_color(y);
            push({ 0.f,  y, -cfg.extent }, { 0.f,  y, cfg.extent }, col);
            push({ 0.f, -y, -cfg.extent }, { 0.f, -y, cfg.extent }, col);
        }
    }
}

void build_axes(std::span<Vertex> out, const AxesConfig& cfg) {
    const u32 needed = axes_vertex_count(cfg);
    if (out.size() < static_cast<std::size_t>(needed))
        throw std::invalid_argument("[build_axes] output span too small");

    u32 idx = 0;
    const float e = cfg.extent;

    out[idx++] = Vertex{ { -e, 0.f, 0.f }, colors::X_AXIS };
    out[idx++] = Vertex{ {  e, 0.f, 0.f }, colors::X_AXIS };
    out[idx++] = Vertex{ { 0.f, -e, 0.f }, colors::Y_AXIS };
    out[idx++] = Vertex{ { 0.f,  e, 0.f }, colors::Y_AXIS };

    if (cfg.is_3d) {
        out[idx++] = Vertex{ { 0.f, 0.f, -e }, colors::Z_AXIS };
        out[idx++] = Vertex{ { 0.f, 0.f,  e }, colors::Z_AXIS };
    }
}

} // namespace ndde::math
