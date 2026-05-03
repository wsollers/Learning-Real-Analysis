#pragma once

#include "app/MultiWellSpawner.hpp"
#include "app/PanelHost.hpp"
#include "app/ParticleSystem.hpp"
#include "app/SurfaceMeshCache.hpp"
#include "app/Viewport.hpp"

namespace ndde {

class MultiWellPanels {
public:
    MultiWellPanels(ParticleSystem& particles,
                    MultiWellSpawner& spawner,
                    SurfaceMeshCache& mesh,
                    Viewport& vp3d,
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
    MultiWellSpawner& m_spawner;
    SurfaceMeshCache& m_mesh;
    Viewport& m_vp3d;
    u32& m_grid_lines;
    float& m_color_scale;
    bool& m_show_frenet;
    bool& m_show_osc;
    bool& m_paused;
    float& m_sim_speed;
    GoalStatus& m_goal_status;

    void register_panels();
    void draw_particles_body();
    void draw_camera_body();
};

} // namespace ndde
