#pragma once
// app/GaussianSurface.hpp
// The multi-Gaussian + sinusoidal test surface:
//
//   f(x,y) = 1.2*exp(-((x-1.5)^2+(y-0.5)^2))
//           - 0.9*exp(-((x+1.2)^2/0.7 + (y+1.0)^2/1.4))
//           + 0.7*exp(-((x-0.3)^2/1.8 + (y-1.7)^2/0.6))
//           + 0.15*sin(2x)*sin(2y)
//
// ── What this provides ────────────────────────────────────────────────────────
// 1. f(x,y)  — scalar surface height
// 2. grad    — analytical gradient via finite differences
// 3. Surface geometry: Gaussian / mean curvature via second fundamental form
// 4. Animated curve that flows across the surface (level-curve tangent walk)
// 5. Frenet frame (T, N, B, kappa, tau) at any curve point
// 6. Flat contour tessellation for the 2D contour view
//
// ── Coordinate conventions ────────────────────────────────────────────────────
// The surface is a graph surface p(x,y) = (x, y, f(x,y)) in R^3.
// The 3D view renders it as a point cloud / wireframe in world-space.
// The 2D contour view renders it in the xy-plane via ortho projection.

#include "math/Scalars.hpp"
#include "renderer/GpuTypes.hpp"
#include <vector>
#include <span>
#include <functional>

namespace ndde {

// ── GaussianSurface ───────────────────────────────────────────────────────────

class GaussianSurface {
public:
    static constexpr f32 XMIN = -4.f;
    static constexpr f32 XMAX =  4.f;
    static constexpr f32 YMIN = -4.f;
    static constexpr f32 YMAX =  4.f;

    // ── Scalar evaluation ─────────────────────────────────────────────────────
    [[nodiscard]] static f32 eval(f32 x, f32 y) noexcept;

    // ── Gradient via central differences ─────────────────────────────────────
    struct Grad { f32 fx, fy; };
    [[nodiscard]] static Grad grad(f32 x, f32 y) noexcept;

    // ── Unit surface normal (−fx, −fy, 1) / |...| ────────────────────────────
    [[nodiscard]] static Vec3 unit_normal(f32 x, f32 y) noexcept;

    // ── Gaussian and mean curvature ───────────────────────────────────────────
    [[nodiscard]] static f32 gaussian_curvature(f32 x, f32 y) noexcept;
    [[nodiscard]] static f32 mean_curvature(f32 x, f32 y) noexcept;

    // ── Tessellation ──────────────────────────────────────────────────────────
    // Wireframe grid: u_lines lines in x, v_lines lines in y.
    // Coloured by height using a 7-stop heatmap (blue → red).
    [[nodiscard]] static u32 wireframe_vertex_count(u32 u_lines, u32 v_lines) noexcept;
    static void tessellate_wireframe(std::span<Vertex> out, u32 u_lines, u32 v_lines);

    // Flat contour lines at the given levels.  Returns vertex count written.
    [[nodiscard]] static u32 contour_max_vertices(u32 grid_n, u32 n_levels) noexcept;
    static u32 tessellate_contours(std::span<Vertex> out,
                                   u32 grid_n,
                                   const float* levels, u32 n_levels,
                                   Vec4 color = {1.f,1.f,1.f,0.7f});

    // ── Height-based colour ───────────────────────────────────────────────────
    [[nodiscard]] static Vec4 height_color(f32 z) noexcept;

    // Global z-range — precomputed on a 100x100 grid.
    static constexpr f32 Z_MIN = -1.6f;
    static constexpr f32 Z_MAX =  1.6f;
};

// ── AnimatedCurve ─────────────────────────────────────────────────────────────
// A parametric curve that walks across the surface.
// State is pure (no virtual dispatch): advance(dt) updates the walk.

struct FrenetFrame {
    Vec3  T = {1.f, 0.f, 0.f};   ///< unit tangent
    Vec3  N = {0.f, 1.f, 0.f};   ///< principal normal
    Vec3  B = {0.f, 0.f, 1.f};   ///< binormal
    f32   kappa = 0.f;             ///< curvature κ
    f32   tau   = 0.f;             ///< torsion τ
    f32   speed = 0.f;             ///< |p'(t)|
};

class AnimatedCurve {
public:
    static constexpr u32  MAX_TRAIL = 1200;
    static constexpr f32  WALK_SPEED = 0.9f;

    void reset();
    void advance(f32 dt, f32 speed_scale = 1.f);

    // Returns Frenet frame at the given trail index.
    [[nodiscard]] FrenetFrame frenet_at(u32 idx) const noexcept;

    // ── Geometry for GPU ─────────────────────────────────────────────────────
    // Trail as a line strip (positions lifted onto surface).
    [[nodiscard]] u32 trail_vertex_count() const noexcept;
    void tessellate_trail(std::span<Vertex> out) const;

    // Head dot (single point).
    [[nodiscard]] Vec3 head_world() const noexcept;

    // ── State query ───────────────────────────────────────────────────────────
    [[nodiscard]] u32  trail_size() const noexcept { return static_cast<u32>(m_trail.size()); }
    [[nodiscard]] bool has_trail()  const noexcept { return m_trail.size() >= 3; }
    [[nodiscard]] Vec3 trail_pt(u32 i) const noexcept { return m_trail[i]; }

private:
    struct WalkState {
        f32 x = -3.2f;
        f32 y = -3.0f;
        f32 phase = 0.f;   // governs the sinusoidal wobble
        f32 angle = 0.f;   // current walk direction (radians in xy-plane)
    };

    WalkState         m_walk;
    std::vector<Vec3> m_trail;   // world-space points ON the surface

    void step(f32 dt, f32 speed_scale);
};

} // namespace ndde
