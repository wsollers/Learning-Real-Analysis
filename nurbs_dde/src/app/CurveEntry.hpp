#pragma once
// app/CurveEntry.hpp
// Owns one renderable curve: geometry, GPU buffer, animation state, UI state.

#include "geometry/Curve.hpp"
#include "math/Vec3.hpp"
#include <memory>
#include <string>
#include <vector>

#include "HoverResult.hpp"

namespace ndde {
enum class CurveType { Parabola, Hyperbola };

struct CurveEntry {
    CurveType   type;
    std::string name;
    bool        enabled       = false;  ///< Checkbox in master panel
    bool        window_open   = true;   ///< Whether the parameter window is open

    // Animation
    float draw_fraction = 0.f;
    bool  playing       = false;

    // Visual
    float colour[3] = {1.f, 1.f, 1.f};  ///< RGB for colour picker

    // Geometry (rebuilt when parameters change)
    std::unique_ptr<geometry::Curve>  curve;
    std::vector<math::Vec4>           verts; ///< CPU-side copy for hover queries

    // Parabola parameters
    float par_a    =  1.f;
    float par_b    =  0.f;
    float par_c    =  0.f;
    float par_tmin = -1.f;
    float par_tmax =  1.f;

    // Hyperbola parameters
    float hyp_a         = 1.f;
    float hyp_b         = 1.f;
    float hyp_h         = 0.f;
    float hyp_k         = 0.f;
    float hyp_range     = 2.f;
    int   hyp_axis      = 0;   // 0 = Horizontal, 1 = Vertical
};



} // namespace ndde::app
