#pragma once
// app/SurfaceSimPanels.hpp
// ImGui panel component for SurfaceSimScene.

#include "app/HotkeyManager.hpp"
#include "app/PanelHost.hpp"
#include "app/ParticleSystem.hpp"
#include "app/SurfaceSimController.hpp"
#include "app/SurfaceSimSceneState.hpp"
#include "app/SurfaceSimSpawner.hpp"
#include "app/Viewport.hpp"
#include "math/Surfaces.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ndde {

class SurfaceSimPanels {
public:
    using ExportFn = std::function<void(const std::string&)>;

    SurfaceSimPanels(std::unique_ptr<ndde::math::ISurface>& surface,
                     ParticleSystem& particles,
                     std::vector<AnimatedCurve>& curves,
                     Viewport& vp3d,
                     Viewport& vp2d,
                     HotkeyManager& hotkeys,
                     SurfaceSelectionState& surface_state,
                     SurfaceDisplayState& display,
                     SurfaceOverlayState& overlays,
                     SurfaceUiState& ui,
                     SurfaceSpawnState& spawn,
                     SurfaceBehaviorParams& behavior,
                     SurfacePursuitState& pursuit,
                     SurfaceSimSpawner& spawner,
                     SurfaceSimController& controller,
                     bool& paused,
                     float& sim_time,
                     float& sim_speed,
                     GoalStatus& goal_status,
                     ExportFn export_session);

    void draw_all();
    void draw_hotkey_panel();
    void register_panels();

private:
    std::unique_ptr<ndde::math::ISurface>& m_surface;
    ParticleSystem& m_particles;
    std::vector<AnimatedCurve>& m_curves;
    Viewport& m_vp3d;
    Viewport& m_vp2d;
    HotkeyManager& m_hotkeys;
    SurfaceSelectionState& m_surface_state;
    SurfaceDisplayState& m_display;
    SurfaceOverlayState& m_overlays;
    SurfaceUiState& m_ui;
    SurfaceSpawnState& m_spawn;
    SurfaceBehaviorParams& m_behavior;
    SurfacePursuitState& m_pursuit;
    SurfaceSimSpawner& m_spawner;
    SurfaceSimController& m_controller;
    bool& m_paused;
    float& m_sim_time;
    float& m_sim_speed;
    GoalStatus& m_goal_status;
    ExportFn m_export_session;
    PanelHost m_host;

    void draw_panel_surface();
    void draw_panel_particles();
    void draw_panel_overlays();
    void draw_panel_brownian();
    void draw_panel_pursuit();
    void draw_panel_geometry();
    void draw_panel_camera();
    void draw_leader_seeker_panel();
};

} // namespace ndde
