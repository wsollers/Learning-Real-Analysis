#pragma once

#include "app/PanelHost.hpp"
#include "app/ParticleSwarmFactory.hpp"
#include "app/ParticleSystem.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "app/Viewport.hpp"
#include "app/WavePredatorPreySpawner.hpp"

namespace ndde {

class WavePredatorPreyPanels {
public:
    WavePredatorPreyPanels(ParticleSystem& particles,
                           WavePredatorPreySpawner& spawner,
                           SurfaceMeshCache& mesh,
                           Viewport& vp3d,
                           SwarmBuildResult& last_swarm,
                           u32& grid_lines,
                           float& color_scale,
                           bool& show_frenet,
                           bool& show_osc,
                           bool& paused,
                           float& sim_speed,
                           GoalStatus& goal_status);

    void draw_all();

private:
    PanelHost m_panels;
    ParticleSystem& m_particles;
    WavePredatorPreySpawner& m_spawner;
    SurfaceMeshCache& m_mesh;
    Viewport& m_vp3d;
    SwarmBuildResult& m_last_swarm;
    u32& m_grid_lines;
    float& m_color_scale;
    bool& m_show_frenet;
    bool& m_show_osc;
    bool& m_paused;
    float& m_sim_speed;
    GoalStatus& m_goal_status;

    void register_panels();
    void draw_swarms_body();
    void draw_camera_body();
};

} // namespace ndde
