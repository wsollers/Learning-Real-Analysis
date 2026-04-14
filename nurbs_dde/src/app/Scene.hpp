#pragma once
// app/Scene.hpp

#include "engine/EngineAPI.hpp"
#include "engine/AppConfig.hpp"
#include "math/Axes.hpp"
#include "math/Conics.hpp"
#include "math/Surfaces.hpp"
#include "app/AnalysisPanel.hpp"
#include "app/HoverResult.hpp"
#include "app/PerformancePanel.hpp"
#include "app/Viewport.hpp"
#include "app/CoordDebugPanel.hpp"

#include <imgui.h>
#include <memory>
#include <vector>
#include <string>
#include <utility>

namespace ndde {

// ── ConicEntry ────────────────────────────────────────────────────────────────
// Holds one curve of any supported type.
// type: 0=Parabola  1=Hyperbola  2=Helix  3=ParaboloidCurve

struct ConicEntry {
    std::string                   name;
    bool                          enabled      = true;
    Vec4                          color        = { 1.f, 1.f, 1.f, 1.f };
    u32                           tessellation = 256;
    std::unique_ptr<math::IConic> conic;
    int                           type         = 0;

    // Parabola params
    float par_a = -1.f, par_b = 0.f, par_c = 0.f;
    float par_tmin = -2.f, par_tmax = 2.f;

    // Hyperbola params
    float hyp_a = 1.f, hyp_b = 1.f, hyp_h = 0.f, hyp_k = 0.f, hyp_range = 2.f;
    bool  hyp_two_branch = true;
    int   hyp_axis       = 0;

    // Helix params
    float hel_radius = 1.f;
    float hel_pitch  = 0.5f;
    float hel_tmin   = 0.f;
    float hel_tmax   = 6.2832f * 2.f;

    // ParaboloidCurve params
    float pc_a     = 1.f;   // must match the Paraboloid's a
    float pc_theta = 0.f;   // azimuthal angle (radians)
    float pc_tmin  = 0.f;
    float pc_tmax  = 1.5f;

    bool needs_rebuild = true;
    void mark_dirty()  { needs_rebuild = true; }

    std::vector<std::pair<float, float>> snap_cache;
    std::vector<float>                   snap_cache3d;
    void rebuild();
};

// ── SurfaceEntry ──────────────────────────────────────────────────────────────
// Holds one parametric 2-manifold surface.
// Currently only type 0 = Paraboloid is implemented.

struct SurfaceEntry {
    std::string                     name;
    bool                            enabled      = true;
    Vec4                            color        = { 0.3f, 0.5f, 0.8f, 0.55f };
    std::unique_ptr<math::ISurface> surface;
    int                             type         = 0;

    // Paraboloid params
    float par_a    = 1.f;
    float par_umax = 1.5f;
    u32   u_lines  = 16;   // wireframe grid density
    u32   v_lines  = 24;

    bool needs_rebuild = true;
    void mark_dirty()  { needs_rebuild = true; }
    void rebuild();
};

// ── Scene ─────────────────────────────────────────────────────────────────────

class Scene {
public:
    explicit Scene(EngineAPI api);
    ~Scene() = default;

    void on_frame();

private:
    EngineAPI                m_api;
    Viewport                 m_vp;
    math::AxesConfig         m_axes_cfg;
    AnalysisPanel            m_analysis_panel;
    PerformancePanel         m_perf_panel;
    HoverResult              m_hover;
    CoordDebugPanel          m_coord_debug;
    std::vector<ConicEntry>  m_conics;
    std::vector<SurfaceEntry> m_surfaces;

    void sync_viewport();
    void update_camera();
    void update_hover();
    void update_hover_3d();

    void submit_grid();
    void submit_axes();
    void submit_conics();
    void submit_surfaces();            ///< wireframe 2-manifolds
    void submit_epsilon_ball();
    void submit_epsilon_sphere();
    void submit_interval_lines();
    void submit_interval_labels();
    void submit_secant_line();
    void submit_tangent_line();
    void submit_frenet_frame();
    void submit_osc_circle();

    void draw_main_panel();
    void draw_conic_panel(ConicEntry& e, int idx);
    void draw_surface_panel(SurfaceEntry& e, int idx);

    void add_parabola();
    void add_hyperbola();
    void add_helix();
    void add_paraboloid();           ///< adds paraboloid surface + its meridional curve together
    void add_paraboloid_curve();     ///< adds a standalone surface curve
};

} // namespace ndde
