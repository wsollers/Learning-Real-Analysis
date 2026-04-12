#pragma once
// app/HoverResult.hpp
// Data produced each frame by curve-hover hit testing.
// Consumed by AnalysisPanel::draw() and all geometry submission in Scene.

namespace ndde {

struct HoverResult {
    // ── Snap point ────────────────────────────────────────────────────────────
    bool  hit       = false;
    float world_x   = 0.f;
    float world_y   = 0.f;
    float world_z   = 0.f;   ///< Always 0 for 2D curves; used by helix
    int   curve_idx = -1;
    int   snap_idx  = -1;
    float snap_t    = 0.f;   ///< Curve parameter at snap point

    // ── Secant (2D) ───────────────────────────────────────────────────────────
    bool  has_secant  = false;
    float secant_x0   = 0.f;
    float secant_y0   = 0.f;
    float secant_x1   = 0.f;
    float secant_y1   = 0.f;
    float slope       = 0.f;
    float intercept   = 0.f;

    // ── Frenet–Serret frame (valid in 2D and 3D) ──────────────────────────────
    // T = unit tangent,  N = unit normal,  B = unit binormal
    // In 2D: B = (0,0,1) always, N lies in the xy-plane
    bool  has_tangent    = false;
    float tangent_slope  = 0.f;  ///< dy/dx (2D only; for readout)
    float T[3]           = {};   ///< Unit tangent vector
    float N[3]           = {};   ///< Unit normal vector (principal normal)
    float B[3]           = {};   ///< Unit binormal vector
    float kappa          = 0.f;  ///< Curvature κ = |p' × p''| / |p'|³
    float tau            = 0.f;  ///< Torsion   τ = (p'×p'')·p''' / |p'×p''|²
    float speed          = 0.f;  ///< |p'(t)| — arc-length rate
    float osc_radius     = 0.f;  ///< Osculating circle radius = 1/κ (0 if κ=0)
};

} // namespace ndde
