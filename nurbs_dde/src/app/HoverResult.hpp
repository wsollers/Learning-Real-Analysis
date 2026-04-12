#pragma once
// app/HoverResult.hpp
// Data produced each frame by curve-hover hit testing.
// Consumed by AnalysisPanel::draw() and the secant/tangent submission in Scene.

namespace ndde {

struct HoverResult {
    bool  hit       = false;
    float world_x   = 0.f;
    float world_y   = 0.f;
    int   curve_idx = -1;
    int   snap_idx  = -1;

    bool  has_secant  = false;
    float secant_x0   = 0.f;
    float secant_y0   = 0.f;
    float secant_x1   = 0.f;
    float secant_y1   = 0.f;
    float slope       = 0.f;
    float intercept   = 0.f;

    bool  has_tangent   = false;
    float tangent_slope = 0.f;
};

} // namespace ndde
