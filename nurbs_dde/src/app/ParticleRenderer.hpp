#pragma once
// app/ParticleRenderer.hpp
// ParticleRenderer: submits all 3D particle geometry to the primary render window.
//
// Extracted from SurfaceSimScene (B2 refactor).  Owns a copy of EngineAPI
// (a value type -- all std::function members, cheap to copy) and all the
// submit_* methods that previously lived in SurfaceSimScene.
//
// Usage
// ─────
// Each frame, SurfaceSimScene syncs the visibility flags from its hotkey state
// and then calls submit_all().  The renderer is stateless between frames
// (it holds no particle data, only the API handle and visibility flags).
//
// Note: this renderer uses RenderTarget::Primary3D only.
// The 2D contour second window (RenderTarget::Contour2D) stays in SurfaceSimScene
// because it has its own MVP and scene-specific rendering logic.

#include "engine/EngineAPI.hpp"
#include "app/AnimatedCurve.hpp"
#include "app/FrenetFrame.hpp"
#include "math/Surfaces.hpp"
#include "math/Scalars.hpp"
#include <vector>

namespace ndde {

class ParticleRenderer {
public:
    explicit ParticleRenderer(EngineAPI api) : m_api(std::move(api)) {}

    // ── Visibility flags ──────────────────────────────────────────────────────
    // Set by SurfaceSimScene from hotkey / panel state before each submit_all().
    bool show_frenet       = true;
    bool show_T            = true;
    bool show_N            = true;
    bool show_B            = true;
    bool show_osc          = true;
    bool show_dir_deriv    = false;
    bool show_normal_plane = false;
    bool show_torsion      = false;
    float frame_scale      = 0.25f;

    // ── Primary entry point ───────────────────────────────────────────────────
    // Submit trails, head dots, and overlays for all curves in one call.
    // snap_curve / snap_idx: trail point the cursor is hovering (active curve).
    // sim_time: passed to surface geometry queries inside overlay methods.
    void submit_all(const std::vector<AnimatedCurve>& curves,
                    const ndde::math::ISurface&        surface,
                    float                               sim_time,
                    const Mat4&                         mvp,
                    int  snap_curve,
                    int  snap_idx,
                    bool snap_on_curve);

    // ── Individual submit methods (public for future composability) ───────────
    void submit_trail_3d     (const AnimatedCurve& c, const Mat4& mvp);
    void submit_head_dot_3d  (const AnimatedCurve& c, const Mat4& mvp);

    void submit_frenet_3d    (const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);
    void submit_osc_circle_3d(const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);

    // Surface-dependent overlays receive explicit surface + sim_time parameters
    // (they previously read m_surface / m_sim_time from SurfaceSimScene).
    void submit_surface_frame_3d(const AnimatedCurve&        c,
                                 u32                         trail_idx,
                                 const Mat4&                 mvp,
                                 const ndde::math::ISurface& surface,
                                 float                       sim_time);

    void submit_normal_plane_3d (const AnimatedCurve&        c,
                                 u32                         trail_idx,
                                 const Mat4&                 mvp,
                                 const ndde::math::ISurface& surface,
                                 float                       sim_time);

    void submit_torsion_3d   (const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);

private:
    EngineAPI m_api;

    void submit_arrow(Vec3 origin, Vec3 dir, Vec4 color, float length,
                      const Mat4& mvp, Topology topo = Topology::LineList);
};

} // namespace ndde
