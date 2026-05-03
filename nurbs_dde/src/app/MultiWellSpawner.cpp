#include "app/MultiWellSpawner.hpp"

#include "app/ParticleBehaviors.hpp"
#include "numeric/ops.hpp"

#include <utility>

namespace ndde {

MultiWellSpawner::MultiWellSpawner(ParticleSystem& particles,
                                   u32& spawn_count,
                                   float& sim_time,
                                   GoalStatus& goal_status) noexcept
    : m_particles(particles)
    , m_spawn_count(spawn_count)
    , m_sim_time(sim_time)
    , m_goal_status(goal_status)
{}

void MultiWellSpawner::clear_all() noexcept {
    m_particles.clear();
    m_particles.clear_goals();
    m_spawn_count = 0;
    m_goal_status = GoalStatus::Running;
}

void MultiWellSpawner::spawn_showcase_service() {
    clear_all();
    m_sim_time = 0.f;

    spawn_avoider_at({-2.15f, -1.15f}, {0.18f, 0.05f});
    spawn_avoider_at({ 1.55f,  1.35f}, {-0.12f, 0.10f});

    for (int i = 0; i < 90; ++i) {
        m_sim_time += 1.f / 60.f;
        m_particles.update(1.f / 60.f, 1.f, m_sim_time);
    }

    spawn_centroid_seeker_at({-0.20f, 2.55f});
    m_particles.add_goal<CaptureGoal>(CaptureGoal::Params{
        .seeker_role = ParticleRole::Chaser,
        .target_role = ParticleRole::Avoider,
        .radius = 0.20f
    });
    m_goal_status = GoalStatus::Running;
}

void MultiWellSpawner::spawn_avoider() {
    const float a = static_cast<float>(m_spawn_count) * 1.73f;
    const float r = 1.3f + 0.25f * static_cast<float>(m_spawn_count % 3u);
    spawn_avoider_at({r * ops::cos(a), r * ops::sin(a)},
                     {0.10f * ops::cos(a + 1.f), 0.10f * ops::sin(a + 1.f)});
}

void MultiWellSpawner::spawn_avoider_at(glm::vec2 uv, glm::vec2 drift) {
    AvoidParticleBehavior::Params avoid;
    avoid.target = TargetSelector::nearest(ParticleRole::Chaser);
    avoid.speed = 0.55f;
    avoid.delay_seconds = 0.35f;

    ParticleBuilder builder = m_particles.factory().particle();
    builder
        .named("Avoider - Delayed Avoid - Brownian")
        .role(ParticleRole::Avoider)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.012f})
        .stochastic()
        .with_behavior<AvoidParticleBehavior>(avoid)
        .with_behavior<ConstantDriftBehavior>(0.35f, drift)
        .with_behavior<BrownianBehavior>(0.35f, BrownianBehavior::Params{
            .sigma = 0.10f,
            .drift_strength = 0.04f
        });
    m_particles.spawn(std::move(builder));
    ++m_spawn_count;
}

void MultiWellSpawner::spawn_centroid_seeker() {
    spawn_centroid_seeker_at({0.f, 0.f});
}

void MultiWellSpawner::spawn_centroid_seeker_at(glm::vec2 uv) {
    ParticleBuilder builder = m_particles.factory().particle();
    builder
        .named("Chaser - Centroid Seek - Brownian")
        .role(ParticleRole::Chaser)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.012f})
        .stochastic()
        .with_behavior<CentroidSeekBehavior>(CentroidSeekBehavior::Params{
            .role = ParticleRole::Avoider,
            .speed = 0.78f
        })
        .with_behavior<BrownianBehavior>(0.18f, BrownianBehavior::Params{
            .sigma = 0.055f,
            .drift_strength = 0.f
        });
    m_particles.spawn(std::move(builder));
    ++m_spawn_count;
}

} // namespace ndde
