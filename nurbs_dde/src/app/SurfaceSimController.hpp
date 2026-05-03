#pragma once
// app/SurfaceSimController.hpp
// Surface lifecycle and simulation stepping service for SurfaceSimScene.

#include "app/ParticleSystem.hpp"
#include "app/SurfaceSimSceneState.hpp"
#include "app/SurfaceSimSpawner.hpp"
#include "math/Surfaces.hpp"

#include <memory>
#include <vector>

namespace ndde {

class SurfaceSimController {
public:
    SurfaceSimController(std::unique_ptr<ndde::math::ISurface>& surface,
                         ParticleSystem& particles,
                         std::vector<AnimatedCurve>& curves,
                         SurfaceSelectionState& surface_state,
                         SurfaceDisplayState& display,
                         SurfacePursuitState& pursuit,
                         SurfaceSpawnState& spawn,
                         SurfaceSimSpawner& spawner,
                         bool& paused,
                         float& sim_time,
                         float& sim_speed) noexcept;

    void swap_surface(SurfaceType type);
    void advance_simulation(f32 dt);
    void sync_pairwise_constraints();
    void rebuild_extremum_table_if_needed();

private:
    std::unique_ptr<ndde::math::ISurface>& m_surface;
    ParticleSystem& m_particles;
    std::vector<AnimatedCurve>& m_curves;
    SurfaceSelectionState& m_surface_state;
    SurfaceDisplayState& m_display;
    SurfacePursuitState& m_pursuit;
    SurfaceSpawnState& m_spawn;
    SurfaceSimSpawner& m_spawner;
    bool& m_paused;
    float& m_sim_time;
    float& m_sim_speed;
};

} // namespace ndde
