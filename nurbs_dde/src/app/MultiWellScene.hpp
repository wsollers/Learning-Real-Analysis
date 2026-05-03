#pragma once
// app/MultiWellScene.hpp
// Sim 3: multi-well Gaussian/trigonometric landscape with centroid pursuit.

#include "engine/EngineAPI.hpp"
#include "app/HotkeyManager.hpp"
#include "app/MultiWellPanels.hpp"
#include "app/MultiWellSpawner.hpp"
#include "app/ParticleSystem.hpp"
#include "app/SimulationSceneBase.hpp"
#include "app/SurfaceSceneView.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "app/SurfaceRegistry.hpp"
#include "app/Viewport.hpp"

#include <memory>
#include <string_view>

namespace ndde {

class MultiWellScene final : public SimulationSceneBase {
public:
    explicit MultiWellScene(EngineAPI api);

    void on_frame(f32 dt) override;
    void on_key_event(int key, int action, int mods) override;
    [[nodiscard]] std::string_view name() const override { return "Multi-Well Centroid"; }

private:
    EngineAPI m_api;
    std::unique_ptr<MultiWellWaveSurface> m_surface;
    ParticleSystem m_particles;
    MultiWellSpawner m_spawner;
    HotkeyManager m_hotkeys;
    Viewport m_vp3d;

    u32 m_grid_lines = 70;
    float m_color_scale = 1.25f;
    u32 m_spawn_count = 0;
    bool m_show_frenet = true;
    bool m_show_osc = true;

    SurfaceMeshCache m_mesh;
    SurfaceSceneView m_view;
    MultiWellPanels m_panels;

    void register_bindings();
};

} // namespace ndde
