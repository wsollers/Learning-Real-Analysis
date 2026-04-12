#pragma once
// app/Scene.hpp

#include "engine/EngineAPI.hpp"
#include "engine/AppConfig.hpp"
#include "math/Axes.hpp"
#include "math/Conics.hpp"
#include "app/AnalysisPanel.hpp"
#include "app/HoverResult.hpp"
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
// type: 0=Parabola  1=Hyperbola  2=Helix

struct ConicEntry {
    std::string                   name;
    bool                          enabled      = true;
    Vec4                          color        = { 1.f, 1.f, 1.f, 1.f };
    u32                           tessellation = 256;
    std::unique_ptr<math::IConic> conic;
    int                           type         = 0;

    // ── Parabola params ───────────────────────────────────────────────────────
    float par_a = -1.f, par_b = 0.f, par_c = 0.f;
    float par_tmin = -2.f, par_tmax = 2.f;

    // ── Hyperbola params ──────────────────────────────────────────────────────
    float hyp_a = 1.f, hyp_b = 1.f, hyp_h = 0.f, hyp_k = 0.f, hyp_range = 2.f;
    bool  hyp_two_branch = true;
    int   hyp_axis       = 0;

    // ── Helix params ──────────────────────────────────────────────────────────
    float hel_radius = 1.f;
    float hel_pitch  = 0.5f;
    float hel_tmin   = 0.f;
    float hel_tmax   = 6.2832f * 2.f;  // 2 full revolutions

    bool needs_rebuild = true;
    void mark_dirty()  { needs_rebuild = true; }

    std::vector<std::pair<float, float>> snap_cache;   ///< (x, y)  — 2D only
    std::vector<float>                   snap_cache3d;  ///< x,y,z interleaved — 3D
    void rebuild();
};

// ── Scene ─────────────────────────────────────────────────────────────────────

class Scene {
public:
    explicit Scene(EngineAPI api);
    ~Scene() = default;

    void on_frame();

private:
    EngineAPI               m_api;
    Viewport                m_vp;
    math::AxesConfig        m_axes_cfg;
    AnalysisPanel           m_analysis_panel;
    HoverResult             m_hover;
    CoordDebugPanel         m_coord_debug;
    std::vector<ConicEntry> m_conics;

    // ── Update ────────────────────────────────────────────────────────────────
    void sync_viewport();
    void update_camera();
    void update_hover();       ///< 2D snap
    void update_hover_3d();    ///< 3D snap (helix)

    // ── Geometry ─────────────────────────────────────────────────────────────
    void submit_grid();
    void submit_axes();
    void submit_conics();

    // 2D analysis overlays
    void submit_epsilon_ball();
    void submit_epsilon_sphere();  ///< 3D: latitude/longitude wireframe sphere
    void submit_interval_lines();
    void submit_interval_labels();
    void submit_secant_line();
    void submit_tangent_line();

    // Frenet–Serret overlays (2D + 3D)
    void submit_frenet_frame();    ///< T, N, B arrows at snap point
    void submit_osc_circle();      ///< Osculating circle (2D) or sphere (3D)

    // ── UI ────────────────────────────────────────────────────────────────────
    void draw_main_panel();
    void draw_conic_panel(ConicEntry& e, int idx);

    void add_parabola();
    void add_hyperbola();
    void add_helix();
};

} // namespace ndde
