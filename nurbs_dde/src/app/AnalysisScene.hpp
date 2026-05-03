#pragma once
// app/AnalysisScene.hpp
// AnalysisScene: differential geometry analysis on the SineRationalSurface.

#include "engine/EngineAPI.hpp"
#include "app/AnalysisPanels.hpp"
#include "app/HotkeyManager.hpp"
#include "app/ParticleSystem.hpp"
#include "app/AnalysisSpawner.hpp"
#include "app/SimulationSceneBase.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "app/SurfaceRegistry.hpp"
#include "app/SurfaceSceneView.hpp"
#include "app/Viewport.hpp"
#include "sim/LevelCurveWalker.hpp"

#include <memory>
#include <string_view>

namespace ndde {

class AnalysisScene final : public SimulationSceneBase {
public:
    explicit AnalysisScene(EngineAPI api);

    void on_frame(f32 dt) override;
    void on_key_event(int key, int action, int mods) override;
    [[nodiscard]] std::string_view name() const override { return "Analysis - Sine-Rational"; }

private:
    EngineAPI m_api;
    std::unique_ptr<ndde::math::SineRationalSurface> m_surface;
    ParticleSystem m_particles;
    AnalysisSpawner m_spawner;
    HotkeyManager m_hotkeys;
    Viewport m_vp3d;

    SurfaceMeshCache m_mesh;
    SurfaceSceneView m_view;
    u32 m_grid_lines = 60;
    float m_curv_scale = 3.f;
    bool m_show_frenet = true;
    bool m_show_osc = true;

    float m_epsilon = 0.15f;
    float m_walk_speed = 0.7f;
    float m_noise_sigma = 0.0f;
    u32 m_spawn_count = 0;
    AnalysisPanels m_panels;

    void register_bindings();

    [[nodiscard]] static ndde::sim::LevelCurveWalker* level_walker(AnimatedCurve& c) noexcept;
    [[nodiscard]] static const ndde::sim::LevelCurveWalker* level_walker(const AnimatedCurve& c) noexcept;
};

} // namespace ndde
