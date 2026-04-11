// geometry/Axes2D.cpp
#include "Axes2D.hpp"
#include <cmath>

namespace ndde::geometry {

std::vector<math::Vec4> build_axes(const AxesConfig& cfg) {
    std::vector<math::Vec4> verts;

    auto push = [&](float x, float y) {
        verts.emplace_back(x, y, 0.f, 1.f);
    };

    // ── X axis ────────────────────────────────────────────────────────────────
    push(-cfg.extent, 0.f);
    push( cfg.extent, 0.f);

    // ── Y axis ────────────────────────────────────────────────────────────────
    push(0.f, -cfg.extent);
    push(0.f,  cfg.extent);

    // ── Tick marks ────────────────────────────────────────────────────────────
    if (cfg.tick_spacing > 0.f) {
        const int n = static_cast<int>(cfg.extent / cfg.tick_spacing);
        for (int i = 1; i <= n; ++i) {
            float pos = static_cast<float>(i) * cfg.tick_spacing;

            // X axis ticks (positive and negative)
            for (float x : {pos, -pos}) {
                push(x, -cfg.tick_size);
                push(x,  cfg.tick_size);
            }
            // Y axis ticks (positive and negative)
            for (float y : {pos, -pos}) {
                push(-cfg.tick_size, y);
                push( cfg.tick_size, y);
            }
        }
    }

    return verts;
}

} // namespace ndde::geometry
