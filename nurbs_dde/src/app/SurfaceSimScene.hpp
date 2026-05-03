#pragma once
// app/SurfaceSimScene.hpp
// Thin orchestrator: owns simulation state, delegates rendering and spawning.
//
// B1: FrenetFrame / SurfaceFrame / AnimatedCurve split to their own headers.
// B2: All submit_* particle methods extracted to ParticleRenderer.
// Particle spawning now flows through ParticleSystem / ParticleFactory.
//
// Responsibilities that remain here:
//   - Own m_surface, m_curves, integrators, equations
//   - Call advance_simulation(dt)
//   - Call m_particle_renderer.submit_all()
//   - Call scene-specific PanelHost panels + draw_hotkey_panel()
//   - Surface wireframe + filled geometry caches
//   - Camera orbit + canvas MVP
//   - submit_contour_second_window() (uses submit2, scene-specific 2D MVP)
//   - export_session()
//
// Hotkeys:
//   Ctrl+L  -- spawn a new Leader particle  (blue trail)
//   Ctrl+C  -- spawn a new Chaser particle  (red trail)
//   Ctrl+B  -- spawn a Brownian motion particle (Milstein integrator)
//   Ctrl+R  -- spawn a delay-pursuit chaser targeting curve 0
//   Ctrl+A  -- spawn leader seeker (first press) / pursuit particle (subsequent)
//   Ctrl+F  -- toggle Frenet frame  (T, N, B arrows + osculating circle)
//   Ctrl+D  -- toggle surface frame (Dx, Dy tangents + kappa_n / kappa_g readout)
//   Ctrl+N  -- toggle normal plane patch at particle
//   Ctrl+P  -- pause / unpause simulation
//   Ctrl+T  -- toggle torsion visualisation (tau ribbon + signed readout)
//   Ctrl+Q  -- toggle coordinate debug panel
//   Ctrl+H  -- toggle hotkey reference panel

#include "engine/EngineAPI.hpp"
#include "engine/IScene.hpp"
#include "app/GaussianSurface.hpp"
#include "app/GaussianRipple.hpp"
#include "app/FrenetFrame.hpp"        // FrenetFrame, SurfaceFrame, make_surface_frame
#include "app/AnimatedCurve.hpp"      // AnimatedCurve
#include "sim/BrownianMotion.hpp"
#include "sim/MilsteinIntegrator.hpp"
#include "sim/HistoryBuffer.hpp"
#include "sim/DelayPursuitEquation.hpp"
#include "sim/IConstraint.hpp"
#include "sim/MinDistConstraint.hpp"
#include "math/ExtremumTable.hpp"
#include "math/ExtremumSurface.hpp"
#include "sim/LeaderSeekerEquation.hpp"
#include "sim/BiasedBrownianLeader.hpp"
#include "sim/DirectPursuitEquation.hpp"
#include "sim/MomentumBearingEquation.hpp"
#include "app/Viewport.hpp"
#include "app/HoverResult.hpp"
#include "app/CoordDebugPanel.hpp"
#include "app/PanelHost.hpp"
#include "app/ParticleInspectorPanel.hpp"
#include "app/ParticleRenderer.hpp"  // B2: extracted particle rendering
#include "app/ParticleSystem.hpp"
#include "app/SurfaceMeshCache.hpp"

#include "app/HotkeyManager.hpp"  // decoupled hotkey registration and dispatch
#include <imgui.h>
#include <memory>
#include <vector>
#include <string>

namespace ndde {

class SurfaceSimScene : public IScene {
public:
    explicit SurfaceSimScene(EngineAPI api);
    ~SurfaceSimScene() = default;

    void on_frame(f32 dt) override;
    void on_key_event(int key, int action, int mods) override;
    [[nodiscard]] std::string_view name() const override { return "Surface Simulation"; }
    void set_paused(bool paused) override { m_sim_paused = paused; }
    [[nodiscard]] bool paused() const noexcept override { return m_sim_paused; }

private:
    EngineAPI                              m_api;
    HotkeyManager                          m_hotkeys;           // registered in constructor
    std::unique_ptr<ndde::math::ISurface>  m_surface;   // Step 3: owns the surface
    ParticleSystem                         m_particles;
    std::vector<AnimatedCurve>&            m_curves;    ///< all active particles
    ParticleRenderer                       m_particle_renderer; // B2
    ndde::sim::BrownianMotion::Params      m_bm_params;   // Step 9: Brownian UI params
    ndde::sim::DelayPursuitEquation::Params m_dp_params;  // Step 10: delay-pursuit params
    u32 m_dp_count = 0;                                   // Step 10: spawn counter
    PanelHost m_panels;
    CoordDebugPanel  m_coord_debug;

    // Viewports
    Viewport m_vp3d;   // perspective
    Viewport m_vp2d;   // ortho -- used by contour panel + second window

    // Canvas rect for the 3D window (updated each frame)
    ImVec2 m_canvas3d_pos{};
    ImVec2 m_canvas3d_sz{};

    // Surface selector
    enum class SurfaceType : u8 { Gaussian = 0, Torus = 1, GaussianRipple = 2, Extremum = 3 };
    SurfaceType m_surface_type = SurfaceType::Gaussian;
    f32 m_torus_R = 2.0f;
    f32 m_torus_r = 0.7f;
    GaussianRipple::Params m_ripple_params;  ///< UI-editable ripple parameters

    void swap_surface(SurfaceType type);

    // Simulation time
    // Accumulated wall-clock simulation time (paused when m_sim_paused).
    // Passed to ISurface geometry queries and wireframe tessellation
    // for deforming surfaces (is_time_varying() == true).
    f32 m_sim_time = 0.f;

    // Simulation state
    f32  m_sim_speed     = 1.f;
    bool m_sim_paused    = false;
    f32  m_frame_scale   = 0.25f;
    u32  m_grid_lines    = 64;
    bool m_show_contours = true;

    // Visibility toggles (hotkeys)
    bool m_show_frenet       = true;   ///< Ctrl+F
    bool m_show_dir_deriv    = false;  ///< Ctrl+D -- surface frame Dx/Dy
    bool m_show_normal_plane = false;  ///< Ctrl+P -- normal plane patch
    bool m_show_torsion      = false;  ///< Ctrl+T -- torsion ribbon
    bool m_debug_open        = false;  ///< Ctrl+Q
    bool m_hotkey_panel_open = false;  ///< Ctrl+H

    // Frenet sub-toggles (Simulation panel checkboxes)
    bool m_show_T   = true;
    bool m_show_N   = true;
    bool m_show_B   = true;
    bool m_show_osc = true;

    // Counters for colour-slot cycling within each role
    u32 m_leader_count = 0;
    u32 m_chaser_count = 0;

    // Pairwise constraints
    // Applied once per ordered pair (i, j) with i < j, after per-particle step.
    std::vector<std::unique_ptr<ndde::sim::IPairConstraint>> m_pair_constraints;
    bool  m_pair_collision = false;  ///< UI toggle for MinDistConstraint
    float m_min_dist       = 0.3f;   ///< UI-editable minimum distance

    // Ctrl+A: ExtremumSurface + leader seeker
    ndde::math::ExtremumTable          m_extremum_table;             ///< stable address (value member)
    u32                                m_extremum_rebuild_countdown = 0;

    enum class LeaderMode : u8 { Deterministic = 0, StochasticBiased = 1 };
    LeaderMode m_leader_mode = LeaderMode::Deterministic;
    ndde::sim::LeaderSeekerEquation::Params m_ls_params;
    ndde::sim::BiasedBrownianLeader::Params m_bbl_params;

    enum class PursuitMode : u8 { Direct = 0, Delayed = 1, Momentum = 2 };
    PursuitMode m_pursuit_mode   = PursuitMode::Direct;
    float       m_pursuit_tau    = 1.5f;   ///< for Strategy B (DelayPursuit)
    float       m_pursuit_window = 1.5f;   ///< for Strategy C (MomentumBearing)

    bool m_spawning_pursuer = false;  ///< true after first Ctrl+A spawns a leader
    GoalStatus m_goal_status = GoalStatus::Running;

    // Hover / snap
    int  m_snap_curve    = 0;     ///< which curve the snap belongs to
    int  m_snap_idx      = -1;
    bool m_snap_on_curve = false;
    HoverResult m_hover;

    // Surface display mode
    // Wireframe: line grid (LineList)  -- the existing mode
    // Filled:    triangle mesh coloured by Gaussian curvature K
    // Both:      filled mesh with wireframe overlaid at reduced opacity
    enum class SurfaceDisplay : u8 { Wireframe = 0, Filled = 1, Both = 2 };
    SurfaceDisplay m_surface_display = SurfaceDisplay::Wireframe;

    // Filled surface curvature colour scale
    // K > 0 (elliptic / sphere-like)  -> warm   (amber -> red)
    // K = 0 (flat / parabolic)        -> neutral (grey)
    // K < 0 (hyperbolic / saddle)     -> cool   (teal -> blue)
    // k_curv_scale: maps K to [-1, 1].  Adjust per-surface so the full range
    // of colour is used.  Exposed as a UI slider.
    f32 m_curv_scale = 2.f;   ///< world units^-2 per unit colour range

    SurfaceMeshCache m_mesh;


    void advance_simulation(f32 dt);
    void apply_pairwise_constraints();
    void spawn_showcase_service();
    [[nodiscard]] glm::vec2 reference_uv() const noexcept;
    [[nodiscard]] glm::vec2 offset_spawn(glm::vec2 ref_uv, float radius, float angle) const noexcept;
    void prewarm_particle(AnimatedCurve& particle, int frames = 60);
    void spawn_gradient_particle(ParticleRole role, glm::vec2 uv);
    void spawn_brownian_particle(glm::vec2 uv);
    void spawn_delay_pursuit_particle(glm::vec2 uv);
    void spawn_leader_seeker();
    void spawn_pursuit_particle();
    void rebuild_extremum_table_if_needed();
    void draw_leader_seeker_panel();
    void register_panels();

    [[nodiscard]] Mat4 canvas_mvp_3d(const ImVec2& cpos,
                                      const ImVec2& csz) const noexcept;

    void draw_panel_surface();    ///< surface selector + display mode body
    void draw_panel_particles();  ///< pause, speed, spawn counts, clear, export body
    void draw_panel_overlays();   ///< Frenet toggles, overlay checkboxes, torsion readout body
    void draw_panel_brownian();   ///< sigma, drift, RNG, live sliders body
    void draw_panel_pursuit();    ///< delay pursuit + collision + leader seeker body
    void draw_panel_geometry();   ///< at-head Frenet / curvature / 1st-fund-form readout body
    void draw_panel_camera();     ///< yaw, pitch, zoom, reset body
    void draw_hotkey_panel();
    void draw_surface_3d_window();
    void draw_contour_2d_window();
    void export_session(const std::string& path) const;

    void submit_wireframe_3d(const Mat4& mvp);
    void submit_filled_3d(const Mat4& mvp);

    void submit_contour_second_window();

    void update_hover(const ImVec2& canvas_pos, const ImVec2& canvas_size,
                      bool is_3d);

    static constexpr f32 k_levels[] = {
        -1.2f,-0.9f,-0.6f,-0.3f, 0.f, 0.3f, 0.6f, 0.9f, 1.2f
    };
    static constexpr u32 k_n_levels = 9u;
};

} // namespace ndde
