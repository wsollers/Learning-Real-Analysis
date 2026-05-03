#pragma once
// app/WavePredatorPreyScene.hpp
// Sim 4: trigonometric wave surface with predator/prey swarm recipes.

#include "engine/EngineAPI.hpp"
#include "app/HotkeyManager.hpp"
#include "app/ParticleSwarmFactory.hpp"
#include "app/SimulationSceneBase.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "app/SurfaceRegistry.hpp"
#include "app/SurfaceSceneView.hpp"
#include "app/Viewport.hpp"
#include "app/WavePredatorPreyPanels.hpp"
#include "app/WavePredatorPreySpawner.hpp"

#include <memory>
#include <string_view>

namespace ndde {

class WavePredatorPreyScene final : public SimulationSceneBase {
public:
    explicit WavePredatorPreyScene(EngineAPI api);

    void on_frame(f32 dt) override;
    void on_key_event(int key, int action, int mods) override;
    [[nodiscard]] std::string_view name() const override { return "Wave Predator-Prey"; }

private:
    EngineAPI m_api;
    std::unique_ptr<WavePredatorPreySurface> m_surface;
    ParticleSystem m_particles;
    WavePredatorPreySpawner m_spawner;
    HotkeyManager m_hotkeys;
    Viewport m_vp3d;
    SurfaceMeshCache m_mesh;
    SurfaceSceneView m_view;

    u32 m_grid_lines = 80;
    float m_color_scale = 1.05f;
    bool m_show_frenet = true;
    bool m_show_osc = true;
    SwarmBuildResult m_last_swarm;
    WavePredatorPreyPanels m_panels;

    void register_bindings();
};

} // namespace ndde
