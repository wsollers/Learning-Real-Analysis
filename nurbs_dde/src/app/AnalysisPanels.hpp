#pragma once

#include "app/AnalysisSpawner.hpp"
#include "app/PanelHost.hpp"
#include "app/ParticleSystem.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "app/Viewport.hpp"

namespace ndde {

class AnalysisPanels {
public:
    AnalysisPanels(ndde::math::SineRationalSurface& surface,
                   ParticleSystem& particles,
                   AnalysisSpawner& spawner,
                   SurfaceMeshCache& mesh,
                   Viewport& vp3d,
                   u32& grid_lines,
                   float& curv_scale,
                   bool& show_frenet,
                   bool& show_osc,
                   float& epsilon,
                   float& walk_speed,
                   float& noise_sigma,
                   bool& paused,
                   float& sim_speed,
                   u32& spawn_count,
                   GoalStatus& goal_status);

    void draw_all();

private:
    PanelHost m_panels;
    ndde::math::SineRationalSurface& m_surface;
    ParticleSystem& m_particles;
    AnalysisSpawner& m_spawner;
    SurfaceMeshCache& m_mesh;
    Viewport& m_vp3d;
    u32& m_grid_lines;
    float& m_curv_scale;
    bool& m_show_frenet;
    bool& m_show_osc;
    float& m_epsilon;
    float& m_walk_speed;
    float& m_noise_sigma;
    bool& m_paused;
    float& m_sim_speed;
    u32& m_spawn_count;
    GoalStatus& m_goal_status;

    void register_panels();
    void draw_surface_body();
    void draw_walkers_body();
    void draw_camera_body();
};

} // namespace ndde
