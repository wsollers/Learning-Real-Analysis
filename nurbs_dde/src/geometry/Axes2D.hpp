#pragma once
// geometry/Axes2D.hpp
// Generates vertex data for X and Y axes in R^2, plus optional tick marks.
// Returns two separate line lists (one per axis) suitable for LINE_LIST topology.

#include "math/Vec3.hpp"
#include <vector>

namespace ndde::geometry {

struct AxesConfig {
    float extent      = 1.2f;  ///< How far each axis extends in each direction
    float tick_spacing= 0.25f; ///< Distance between tick marks (0 = no ticks)
    float tick_size   = 0.03f; ///< Half-length of each tick mark
};

/// Returns a flat vec4 buffer for LINE_LIST:
///   X axis: (-extent,0,0,1) → (extent,0,0,1)
///   Y axis: (0,-extent,0,1) → (0,extent,0,1)
///   Plus tick marks on both axes.
std::vector<math::Vec4> build_axes(const AxesConfig& cfg = {});

} // namespace ndde::geometry
