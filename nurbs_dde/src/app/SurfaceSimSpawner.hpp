#pragma once
// app/SurfaceSimSpawner.hpp
// Particle construction service for the legacy Surface Simulation scene.

#include "app/AnimatedCurve.hpp"
#include "app/ParticleGoals.hpp"
#include "app/ParticleSystem.hpp"
#include "app/SurfaceSimSceneState.hpp"
#include "math/Surfaces.hpp"

#include <memory>
#include <vector>

namespace ndde {

class SurfaceSimSpawner {
public:
    SurfaceSimSpawner(std::unique_ptr<ndde::math::ISurface>& surface,
                      ParticleSystem& particles,
                      std::vector<AnimatedCurve>& curves,
                      SurfaceBehaviorParams& behavior,
                      SurfacePursuitState& pursuit,
                      SurfaceSpawnState& spawn,
                      GoalStatus& goal_status,
                      float& sim_time,
                      float& sim_speed) noexcept;

    void spawn_showcase_service();
    void spawn_gradient_particle(ParticleRole role, glm::vec2 uv);
    void spawn_brownian_particle(glm::vec2 uv);
    void spawn_delay_pursuit_particle(glm::vec2 uv);
    void spawn_leader_seeker();
    void spawn_pursuit_particle();
    void prewarm_particle(AnimatedCurve& particle, int frames = 60);

    [[nodiscard]] glm::vec2 reference_uv() const noexcept;
    [[nodiscard]] glm::vec2 offset_spawn(glm::vec2 ref_uv, float radius, float angle) const noexcept;

private:
    std::unique_ptr<ndde::math::ISurface>& m_surface;
    ParticleSystem& m_particles;
    std::vector<AnimatedCurve>& m_curves;
    SurfaceBehaviorParams& m_behavior;
    SurfacePursuitState& m_pursuit;
    SurfaceSpawnState& m_spawn;
    GoalStatus& m_goal_status;
    float& m_sim_time;
    float& m_sim_speed;
};

} // namespace ndde
