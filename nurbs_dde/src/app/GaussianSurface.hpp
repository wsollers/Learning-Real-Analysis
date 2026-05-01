#pragma once
// app/GaussianSurface.hpp
// GaussianSurface: 6-Gaussian asymmetric + double sinusoidal ripple height field.
// Implements ndde::math::ISurface -- p(u,v) = (u, v, f(u,v)).
//
// Static helpers (grad, unit_normal, curvature, tessellate_*, height_color)
// remain for the graph-surface-specific rendering in SurfaceSimScene
// (heatmap, contour lines).  They are guarded by dynamic_cast in Step 3.
// They will be progressively removed as the rendering layer is refactored.
//
// FrenetFrame, SurfaceFrame, make_surface_frame, AnimatedCurve are defined
// here temporarily.  They will move to their own headers in later steps.

#include "math/Scalars.hpp"
#include "math/Surfaces.hpp"    // ISurface base class
#include "sim/IEquation.hpp"    // Step 4: equation interface
#include "sim/IIntegrator.hpp"  // Step 5: integrator interface
#include "sim/HistoryBuffer.hpp" // Step 10: for delay-pursuit recording
#include "renderer/GpuTypes.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <span>
#include <memory>
#include <functional>

namespace ndde {

// == GaussianSurface ==========================================================
// Implements ISurface as a height-field graph:  p(u,v) = (u, v, f(u,v))
// where f is the 6-Gaussian asymmetric + double sinusoidal ripple function.

class GaussianSurface : public ndde::math::ISurface {
public:
    static constexpr f32 XMIN = -6.f;
    static constexpr f32 XMAX =  6.f;
    static constexpr f32 YMIN = -6.f;
    static constexpr f32 YMAX =  6.f;
    static constexpr f32 Z_MIN = -2.0f;
    static constexpr f32 Z_MAX =  2.2f;

    // ── ISurface overrides ────────────────────────────────────────────────────
    // These are the new polymorphic entry points.  Internally they delegate
    // to the static methods below so all existing call sites keep working.

    [[nodiscard]] Vec3  evaluate(float u, float v, float t = 0.f) const override {
        (void)t;
        return Vec3{ u, v, eval_static(u, v) };
    }
    [[nodiscard]] Vec3  du(float u, float v, float t = 0.f) const override;
    [[nodiscard]] Vec3  dv(float u, float v, float t = 0.f) const override;

    [[nodiscard]] float u_min(float = 0.f) const override { return XMIN; }
    [[nodiscard]] float u_max(float = 0.f) const override { return XMAX; }
    [[nodiscard]] float v_min(float = 0.f) const override { return YMIN; }
    [[nodiscard]] float v_max(float = 0.f) const override { return YMAX; }

    // ── Static helpers (unchanged -- existing call sites keep working) ─────────
    // eval_static() is the renamed form of the old eval().
    // The old name eval() now resolves to the ISurface::evaluate() override,
    // but static callers used GaussianSurface::eval(x,y) which is kept below
    // as a forwarding alias so nothing breaks before Step 3.
    [[nodiscard]] static f32  eval_static(f32 x, f32 y) noexcept;

    // Backward-compat alias: GaussianSurface::eval(x,y) still works.
    // Will be removed in Step 3 once all call sites migrate to ISurface.
    [[nodiscard]] static f32  eval(f32 x, f32 y) noexcept { return eval_static(x, y); }

    struct Grad { f32 fx, fy; };
    [[nodiscard]] static Grad grad(f32 x, f32 y) noexcept;
    [[nodiscard]] static Vec3 unit_normal(f32 x, f32 y) noexcept;
    [[nodiscard]] static f32  gaussian_curvature(f32 x, f32 y) noexcept;
    [[nodiscard]] static f32  mean_curvature(f32 x, f32 y) noexcept;

    [[nodiscard]] static u32 wireframe_vertex_count(u32 u_lines, u32 v_lines) noexcept;
    static void tessellate_wireframe(std::span<Vertex> out, u32 u_lines, u32 v_lines);

    [[nodiscard]] static u32 contour_max_vertices(u32 grid_n, u32 n_levels) noexcept;
    static u32 tessellate_contours(std::span<Vertex> out,
                                   u32 grid_n,
                                   const float* levels, u32 n_levels,
                                   Vec4 color = {1.f,1.f,1.f,0.7f});

    [[nodiscard]] static Vec4 height_color(f32 z) noexcept;
};

// == FrenetFrame ==============================================================

struct FrenetFrame {
    Vec3 T = {1.f, 0.f, 0.f};  ///< unit tangent
    Vec3 N = {0.f, 1.f, 0.f};  ///< principal normal
    Vec3 B = {0.f, 0.f, 1.f};  ///< binormal
    f32  kappa = 0.f;            ///< curvature kappa
    f32  tau   = 0.f;            ///< torsion tau
    f32  speed = 0.f;            ///< |p'(t)|
};

// == SurfaceFrame =============================================================
// Coordinate tangent frame of p(x,y) = (x,y,f(x,y)) at a foot-point.
//
//   Dx = dp/dx = (1, 0, f_x)   |Dx|^2 = E
//   Dy = dp/dy = (0, 1, f_y)   |Dy|^2 = G     Dx.Dy = F
//   n  = unit surface normal
//
// With FrenetFrame:
//   kappa_n = kappa*(N.n)              normal curvature   (surface property)
//   kappa_g = kappa*|N - (N.n)n|      geodesic curvature (curve-on-surface)
//   kappa^2 = kappa_n^2 + kappa_g^2   Meusnier-Pythagorean identity

struct SurfaceFrame {
    Vec3 Dx;              ///< dp/dx -- unnormalized
    Vec3 Dy;              ///< dp/dy -- unnormalized
    Vec3 normal;          ///< unit surface normal
    f32  E = 0.f;         ///< |Dx|^2
    f32  F = 0.f;         ///< Dx.Dy
    f32  G = 0.f;         ///< |Dy|^2
    f32  kappa_n = 0.f;   ///< normal curvature in T-direction
    f32  kappa_g = 0.f;   ///< geodesic curvature
};

// make_surface_frame: compute the coordinate tangent frame at parameter (u,v,t).
// Accepts any ISurface& and a simulation time t so that deforming surfaces
// (IDeformableSurface) return geometry at the correct instant.
// t defaults to 0.f so existing call sites without a time argument still compile.
[[nodiscard]] inline SurfaceFrame
make_surface_frame(const ndde::math::ISurface& surface,
                   f32 u, f32 v,
                   float t = 0.f,
                   const FrenetFrame* fr = nullptr) noexcept
{
    SurfaceFrame sf;
    sf.Dx     = surface.du(u, v, t);
    sf.Dy     = surface.dv(u, v, t);
    sf.E      = glm::dot(sf.Dx, sf.Dx);
    sf.F      = glm::dot(sf.Dx, sf.Dy);
    sf.G      = glm::dot(sf.Dy, sf.Dy);
    sf.normal = surface.unit_normal(u, v, t);
    if (fr && fr->kappa > 1e-6f) {
        const f32 NdotN = glm::dot(fr->N, sf.normal);
        sf.kappa_n = fr->kappa * NdotN;
        sf.kappa_g = fr->kappa * glm::length(fr->N - NdotN * sf.normal);
    }
    return sf;
}

// == AnimatedCurve ============================================================
// One walker particle on the surface.
// SurfaceSimScene owns a std::vector<AnimatedCurve>.
//
// Step 2: receives a const ISurface* (non-owning) at construction.
// Step 4: receives a non-owning IEquation* at construction.
// Step 5: receives a non-owning IIntegrator* at construction.
// Step 5b: optionally owns its equation via m_owned_equation (unique_ptr).
//   If m_owned_equation is set, it shadows m_equation.
//   Use the static factory AnimatedCurve::with_equation(...) to create a
//   particle that owns its own equation instance.
//   Default construction still uses a shared (non-owning) equation pointer,
//   so all existing call sites are unchanged.
//
// Role::Leader  -- Ctrl+L spawns these; trail drawn in shades of blue.
// Role::Chaser  -- Ctrl+C spawns these; trail drawn in shades of red.

class AnimatedCurve {
public:
    enum class Role : u8 { Leader, Chaser };

    static constexpr u32 MAX_TRAIL   = 1200;
    static constexpr f32 WALK_SPEED  = 0.9f;
    static constexpr u32 MAX_SLOTS   = 4u;

    // Standard constructor: all three pointers are non-owning.
    // surface, equation, integrator must outlive this AnimatedCurve.
    explicit AnimatedCurve(f32 start_x, f32 start_y,
                           Role role,
                           u32 colour_slot,
                           const ndde::math::ISurface*   surface,
                           ndde::sim::IEquation*          equation,
                           const ndde::sim::IIntegrator*  integrator);

    // Factory: construct a particle that OWNS its equation.
    // The unique_ptr is moved into m_owned_equation; m_equation is set to
    // m_owned_equation.get().  All other pointers remain non-owning.
    static AnimatedCurve with_equation(
        f32 start_x, f32 start_y,
        Role role, u32 colour_slot,
        const ndde::math::ISurface*          surface,
        std::unique_ptr<ndde::sim::IEquation> owned_equation,
        const ndde::sim::IIntegrator*         integrator);

    // AnimatedCurve is moveable (unique_ptr member) but not copyable.
    AnimatedCurve(const AnimatedCurve&)            = delete;
    AnimatedCurve& operator=(const AnimatedCurve&) = delete;
    AnimatedCurve(AnimatedCurve&&)                 = default;
    AnimatedCurve& operator=(AnimatedCurve&&)      = default;

    void reset();
    void advance(f32 dt, f32 speed_scale = 1.f);

    [[nodiscard]] FrenetFrame frenet_at(u32 idx) const noexcept;

    [[nodiscard]] u32  trail_vertex_count() const noexcept;
    void tessellate_trail(std::span<Vertex> out) const;
    [[nodiscard]] Vec3 head_world() const noexcept;

    [[nodiscard]] u32  trail_size()    const noexcept { return static_cast<u32>(m_trail.size()); }
    [[nodiscard]] bool has_trail()     const noexcept { return m_trail.size() >= 4; }
    [[nodiscard]] Vec3 trail_pt(u32 i) const noexcept { return m_trail[i]; }
    [[nodiscard]] Role role()          const noexcept { return m_role; }
    [[nodiscard]] u32  colour_slot()   const noexcept { return m_colour_slot; }

    // ── History recording (Step 10: delay-pursuit) ────────────────────────────
    // Enable recording of parameter-space positions with timestamps.
    // capacity: max records in the ring buffer
    // dt_min:   minimum time between stored records (rate-limiter, seconds)
    void enable_history(std::size_t capacity = 4096, float dt_min = 1.f/120.f);

    // Push the current head position into the history buffer at time t.
    // Call once per simulation step AFTER advancing the particle.
    void push_history(float t);

    // Query the history buffer for the parameter-space position at time t_past.
    [[nodiscard]] glm::vec2 query_history(float t_past) const;

    // Non-owning access to the history buffer (may be null if not enabled).
    [[nodiscard]] const ndde::sim::HistoryBuffer* history() const noexcept {
        return m_history.get();
    }

    [[nodiscard]] static Vec4 trail_colour(Role role, u32 slot, f32 age_t) noexcept;

    [[nodiscard]] Vec4 head_colour() const noexcept {
        return trail_colour(m_role, m_colour_slot, 1.f);
    }

private:
    struct WalkState { f32 x, y, phase = 0.f, angle = 0.f; };

    WalkState                              m_walk;
    std::vector<Vec3>                       m_trail;
    const ndde::math::ISurface*             m_surface;       // non-owning, never null
    ndde::sim::IEquation*                   m_equation;      // non-owning OR alias to m_owned_equation
    std::unique_ptr<ndde::sim::IEquation>   m_owned_equation; // null when using shared equation
    const ndde::sim::IIntegrator*           m_integrator;    // non-owning, never null
    std::unique_ptr<ndde::sim::HistoryBuffer> m_history;     // null unless enable_history() called
    Role                                    m_role;
    u32                                     m_colour_slot;
    f32                                     m_start_x, m_start_y;

    void step(f32 dt, f32 speed_scale);
};

} // namespace ndde
