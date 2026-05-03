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
//   - Ask SurfaceSimController to advance simulation
//   - Delegate surface and particle drawing to SurfaceSimView
//   - Call scene-specific SurfaceSimPanels + draw_hotkey_panel()
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
#include "app/ParticleSystem.hpp"
#include "app/SurfaceSimController.hpp"
#include "app/SurfaceSimPanels.hpp"
#include "app/SurfaceSimSceneState.hpp"
#include "app/SurfaceSimSpawner.hpp"
#include "app/SurfaceSimView.hpp"

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
    SurfaceSimSpawner                      m_spawner;
    SurfaceSimController                   m_controller;
    SurfaceSimPanels                       m_panels;
    SurfaceSimView                         m_view;
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

    void register_bindings();

    [[nodiscard]] SurfaceSimViewContext view_context() noexcept;

    void draw_hotkey_panel();
    void draw_surface_3d_window();
    void draw_contour_2d_window();
    void export_session(const std::string& path) const;

    void submit_contour_second_window();
};

} // namespace ndde
