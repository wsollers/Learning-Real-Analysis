#pragma once

#include "app/ParticleGoals.hpp"
#include "app/ParticleSystem.hpp"

namespace ndde {

class MultiWellSpawner {
public:
    MultiWellSpawner(ParticleSystem& particles,
                     u32& spawn_count,
                     float& sim_time,
                     GoalStatus& goal_status) noexcept;

    void spawn_showcase_service();
    void spawn_avoider();
    void spawn_centroid_seeker();
    void spawn_avoider_at(glm::vec2 uv, glm::vec2 drift);
    void spawn_centroid_seeker_at(glm::vec2 uv);
    void clear_all() noexcept;

private:
    ParticleSystem& m_particles;
    u32& m_spawn_count;
    float& m_sim_time;
    GoalStatus& m_goal_status;
};

} // namespace ndde
