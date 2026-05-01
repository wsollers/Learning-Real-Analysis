#pragma once
// app/SurfaceSimScene.hpp
// Particle simulation scene rendered on a parametric surface.
//
// Step 3: the scene now owns the surface as unique_ptr<ISurface>.
// Particles hold a non-owning ISurface* obtained from m_surface.get().
// Adding a new surface = one line in the constructor. No other files change.
//
// Hotkeys:
//   Ctrl+L  -- spawn a new Leader particle  (blue trail)
//   Ctrl+C  -- spawn a new Chaser particle  (red trail)
//   Ctrl+B  -- spawn a Brownian motion particle (Milstein integrator)
//   Ctrl+R  -- spawn a delay-pursuit chaser targeting curve 0
//   Ctrl+F  -- toggle Frenet frame  (T, N, B arrows + osculating circle)
//   Ctrl+D  -- toggle surface frame (Dx, Dy tangents + kappa_n / kappa_g readout)
//   Ctrl+P  -- toggle normal plane patch at particle
//   Ctrl+T  -- toggle torsion visualisation (tau ribbon + signed readout)
//   Ctrl+Q  -- toggle coordinate debug panel
//   Ctrl+H  -- toggle hotkey reference panel

#include "engine/EngineAPI.hpp"
#include "app/GaussianSurface.hpp"
#include "app/GaussianRipple.hpp"
#include "sim/GradientWalker.hpp"
#include "sim/EulerIntegrator.hpp"
#include "sim/BrownianMotion.hpp"
#include "sim/MilsteinIntegrator.hpp"
#include "sim/HistoryBuffer.hpp"
#include "sim/DelayPursuitEquation.hpp"
#include "app/Viewport.hpp"
#include "app/HoverResult.hpp"
#include "app/PerformancePanel.hpp"
#include "app/CoordDebugPanel.hpp"
#include "math/Surfaces.hpp"

#include <imgui.h>
#include <memory>
#include <vector>
#include <string>

namespace ndde {

class SurfaceSimScene {
public:
    explicit SurfaceSimScene(EngineAPI api);
    ~SurfaceSimScene() = default;

    void on_frame(f32 dt);

private:
    EngineAPI                              m_api;
    std::unique_ptr<ndde::math::ISurface>  m_surface;   // Step 3: owns the surface
    ndde::sim::GradientWalker              m_equation;    // Step 4: default equation
    ndde::sim::EulerIntegrator             m_integrator;  // Step 5: Euler scheme
    ndde::sim::MilsteinIntegrator          m_milstein;    // Step 9: Milstein for SDEs
    ndde::sim::BrownianMotion::Params      m_bm_params;   // Step 9: Brownian UI params
    ndde::sim::DelayPursuitEquation::Params m_dp_params;  // Step 10: delay-pursuit params
    u32 m_dp_count = 0;                                   // Step 10: spawn counter
    std::vector<AnimatedCurve>             m_curves;    ///< all active particles
    PerformancePanel m_perf;
    CoordDebugPanel  m_coord_debug;

    // ── Viewports ─────────────────────────────────────────────────────────────
    Viewport m_vp3d;   // perspective
    Viewport m_vp2d;   // ortho — used by contour panel + second window

    // Canvas rect for the 3D window (updated each frame)
    ImVec2 m_canvas3d_pos{};
    ImVec2 m_canvas3d_sz{};

    // ── Surface selector ──────────────────────────────────────────────────────
    enum class SurfaceType : u8 { Gaussian = 0, Torus = 1, GaussianRipple = 2 };
    SurfaceType m_surface_type = SurfaceType::Gaussian;
    f32 m_torus_R = 2.0f;
    f32 m_torus_r = 0.7f;
    GaussianRipple::Params m_ripple_params;  ///< UI-editable ripple parameters

    void swap_surface(SurfaceType type);

    // ── Simulation time ───────────────────────────────────────────────────────
    // Accumulated wall-clock simulation time (paused when m_sim_paused).
    // Passed to ISurface geometry queries and wireframe tessellation
    // for deforming surfaces (is_time_varying() == true).
    f32 m_sim_time = 0.f;

    // ── Simulation state ──────────────────────────────────────────────────────
    f32  m_sim_speed     = 1.f;
    bool m_sim_paused    = false;
    f32  m_frame_scale   = 0.25f;
    u32  m_grid_lines    = 28;
    bool m_show_contours = true;

    // ── Visibility toggles (hotkeys) ──────────────────────────────────────────
    bool m_show_frenet       = true;   ///< Ctrl+F
    bool m_show_dir_deriv    = false;  ///< Ctrl+D — surface frame Dx/Dy
    bool m_show_normal_plane = false;  ///< Ctrl+P — normal plane patch
    bool m_show_torsion      = false;  ///< Ctrl+T — torsion τ ribbon
    bool m_debug_open        = false;  ///< Ctrl+Q
    bool m_hotkey_panel_open  = false;  ///< Ctrl+H

    // Frenet sub-toggles (Simulation panel checkboxes)
    bool m_show_T   = true;
    bool m_show_N   = true;
    bool m_show_B   = true;
    bool m_show_osc = true;

    // ── Hotkey edge-detection state ───────────────────────────────────────────
    bool m_ctrl_f_prev = false;
    bool m_ctrl_d_prev = false;
    bool m_ctrl_p_prev = false;
    bool m_ctrl_t_prev = false;
    bool m_ctrl_q_prev = false;
    bool m_ctrl_l_prev = false;  ///< Ctrl+L: spawn Leader
    bool m_ctrl_c_prev = false;  ///< Ctrl+C: spawn Chaser
    bool m_ctrl_b_prev = false;  ///< Ctrl+B: spawn Brownian particle
    bool m_ctrl_r_prev = false;  ///< Ctrl+R: spawn delay-pursuit chaser
    bool m_ctrl_h_prev = false;  ///< Ctrl+H: toggle hotkey reference panel

    // Counters for colour-slot cycling within each role
    u32 m_leader_count = 0;
    u32 m_chaser_count = 0;

    // ── Hover / snap ──────────────────────────────────────────────────────────
    int  m_snap_curve   = 0;     ///< which curve the snap belongs to
    int  m_snap_idx     = -1;
    bool m_snap_on_curve= false;
    HoverResult m_hover;

    bool m_wireframe_dirty   = true;
    u32  m_cached_grid_lines = 0;
    bool m_dock_built        = false;

    // ── Internal methods ──────────────────────────────────────────────────────
    void advance_simulation(f32 dt);
    void handle_hotkeys();

    [[nodiscard]] Mat4 canvas_mvp_3d(const ImVec2& cpos,
                                      const ImVec2& csz) const noexcept;

    void draw_simulation_panel();
    void draw_hotkey_panel();          ///< Ctrl+H — floating hotkey reference
    void draw_surface_3d_window();
    void draw_contour_2d_window();   ///< ImDrawList overlay on primary window

    void submit_wireframe_3d(const Mat4& mvp);
    void submit_trail_3d(const AnimatedCurve& c, const Mat4& mvp);
    void submit_head_dot_3d(const AnimatedCurve& c, const Mat4& mvp);
    void submit_frenet_3d(const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);
    void submit_osc_circle_3d(const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);
    void submit_surface_frame_3d(const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);
    void submit_normal_plane_3d(const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);
    void submit_torsion_3d(const AnimatedCurve& c, u32 trail_idx, const Mat4& mvp);

    void submit_contour_second_window();

    void submit_arrow(Vec3 origin, Vec3 dir, Vec4 color, f32 length,
                      const Mat4& mvp, Topology topo = Topology::LineList);

    void update_hover(const ImVec2& canvas_pos, const ImVec2& canvas_size,
                      bool is_3d);

    static constexpr f32 k_levels[] = {
        -1.2f,-0.9f,-0.6f,-0.3f, 0.f, 0.3f, 0.6f, 0.9f, 1.2f
    };
    static constexpr u32 k_n_levels = 9u;
};

} // namespace ndde
