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
#include "app/SimulationSceneBase.hpp"
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
#include "app/SurfaceSimSceneState.hpp"

#include "app/HotkeyManager.hpp"  // decoupled hotkey registration and dispatch
#include <imgui.h>
#include <memory>
#include <vector>
#include <string>

namespace ndde {

class SurfaceSimScene : public SimulationSceneBase {
public:
    explicit SurfaceSimScene(EngineAPI api);
    ~SurfaceSimScene() = default;

    void on_frame(f32 dt) override;
    void on_key_event(int key, int action, int mods) override;
    [[nodiscard]] std::string_view name() const override { return "Surface Simulation"; }

private:
    EngineAPI                              m_api;
    HotkeyManager                          m_hotkeys;           // registered in constructor
    std::unique_ptr<ndde::math::ISurface>  m_surface;   // Step 3: owns the surface
    ParticleSystem                         m_particles;
    std::vector<AnimatedCurve>&            m_curves;    ///< all active particles
    ParticleRenderer                       m_particle_renderer; // B2
    PanelHost m_panels;
    CoordDebugPanel  m_coord_debug;

    // Viewports
    Viewport m_vp3d;   // perspective
    Viewport m_vp2d;   // ortho -- used by contour panel + second window

    SurfaceSelectionState m_surface_state;
    SurfaceDisplayState   m_display;
    SurfaceOverlayState   m_overlays;
    SurfaceUiState        m_ui;
    SurfaceSpawnState     m_spawn;
    SurfaceBehaviorParams m_behavior_params;
    SurfacePursuitState   m_pursuit;
    SurfaceHoverState     m_hover_state;

    void swap_surface(SurfaceType type);

    void advance_simulation(f32 dt);
    void sync_pairwise_constraints();
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
    void register_bindings();
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
