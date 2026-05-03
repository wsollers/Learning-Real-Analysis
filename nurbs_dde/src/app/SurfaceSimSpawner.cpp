#include "app/SurfaceSimSpawner.hpp"

#include "app/ParticleBehaviors.hpp"
#include "app/SimulationContext.hpp"
#include "numeric/ops.hpp"
#include "sim/BrownianMotion.hpp"
#include "sim/DelayPursuitEquation.hpp"
#include "sim/DirectPursuitEquation.hpp"
#include "sim/GradientWalker.hpp"
#include "sim/MomentumBearingEquation.hpp"

#include <algorithm>
#include <cmath>

namespace ndde {

SurfaceSimSpawner::SurfaceSimSpawner(std::unique_ptr<ndde::math::ISurface>& surface,
                                     ParticleSystem& particles,
                                     std::vector<AnimatedCurve>& curves,
                                     SurfaceBehaviorParams& behavior,
                                     SurfacePursuitState& pursuit,
                                     SurfaceSpawnState& spawn,
                                     GoalStatus& goal_status,
                                     float& sim_time,
                                     float& sim_speed) noexcept
    : m_surface(surface)
    , m_particles(particles)
    , m_curves(curves)
    , m_behavior(behavior)
    , m_pursuit(pursuit)
    , m_spawn(spawn)
    , m_goal_status(goal_status)
    , m_sim_time(sim_time)
    , m_sim_speed(sim_speed)
{}

glm::vec2 SurfaceSimSpawner::reference_uv() const noexcept {
    if (m_curves.empty())
        return {(m_surface->u_min() + m_surface->u_max()) * 0.5f,
                (m_surface->v_min() + m_surface->v_max()) * 0.5f};
    return m_curves[0].head_uv();
}

glm::vec2 SurfaceSimSpawner::offset_spawn(glm::vec2 ref_uv, float radius, float angle) const noexcept {
    constexpr float margin = 0.5f;
    return {
        std::clamp(ref_uv.x + radius * ops::cos(angle),
                   m_surface->u_min() + margin, m_surface->u_max() - margin),
        std::clamp(ref_uv.y + radius * ops::sin(angle),
                   m_surface->v_min() + margin, m_surface->v_max() - margin)
    };
}

void SurfaceSimSpawner::prewarm_particle(AnimatedCurve& particle, int frames) {
    SimulationContext context = m_particles.context(m_sim_time);
    particle.set_behavior_context(&context);
    for (int i = 0; i < frames; ++i) {
        context.set_time(m_sim_time + static_cast<float>(i) / 60.f);
        particle.advance(1.f / 60.f, m_sim_speed);
    }
    particle.set_behavior_context(nullptr);
}

void SurfaceSimSpawner::spawn_gradient_particle(ParticleRole role, glm::vec2 uv) {
    ParticleBuilder builder = m_particles.factory().particle();
    builder
        .named(std::string(role_name(role)) + " - Gradient Walker")
        .role(role)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .with_equation(std::make_unique<ndde::sim::GradientWalker>());

    AnimatedCurve& particle = m_particles.spawn(std::move(builder));
    prewarm_particle(particle);

    if (role == ParticleRole::Leader) ++m_spawn.leader_count;
    else if (role == ParticleRole::Chaser) ++m_spawn.chaser_count;
}

void SurfaceSimSpawner::spawn_brownian_particle(glm::vec2 uv) {
    ParticleBuilder builder = m_particles.factory().particle();
    builder
        .named("Chaser - Brownian")
        .role(ParticleRole::Chaser)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_equation(std::make_unique<ndde::sim::BrownianMotion>(m_behavior.brownian));

    AnimatedCurve& particle = m_particles.spawn(std::move(builder));
    prewarm_particle(particle);
    ++m_spawn.chaser_count;
}

void SurfaceSimSpawner::spawn_delay_pursuit_particle(glm::vec2 uv) {
    if (m_curves.empty()) return;
    if (m_curves[0].history() == nullptr) {
        const std::size_t cap =
            static_cast<std::size_t>(std::ceil(m_behavior.delay_pursuit.tau * 120.f * 1.5f)) + 256;
        m_curves[0].enable_history(cap, 1.f / 120.f);
    }

    ParticleBuilder builder = m_particles.factory().particle();
    builder
        .named("Chaser - Delay Pursuit")
        .role(ParticleRole::Chaser)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_equation(std::make_unique<ndde::sim::DelayPursuitEquation>(
            m_curves[0].history(), m_surface.get(), m_behavior.delay_pursuit));
    m_particles.spawn(std::move(builder));
    ++m_spawn.chaser_count;
    ++m_spawn.delay_pursuit_count;
}

void SurfaceSimSpawner::spawn_showcase_service() {
    m_curves.clear();
    m_particles.clear_goals();
    m_spawn.leader_count = 0;
    m_spawn.chaser_count = 0;
    m_spawn.delay_pursuit_count = 0;
    m_spawn.spawning_pursuer = true;

    const std::size_t history_cap =
        static_cast<std::size_t>(std::ceil(2.0f * 120.f * 1.5f)) + 256;

    ParticleBuilder leader_builder = m_particles.factory().particle();
    leader_builder
        .named("Leader - Brownian - Bias Drift")
        .role(ParticleRole::Leader)
        .at({-4.5f, -4.0f})
        .history(history_cap, 1.f / 120.f)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_behavior<ConstantDriftBehavior>(0.75f, glm::vec2{0.55f, 0.28f})
        .with_behavior<BrownianBehavior>(BrownianBehavior::Params{
            .sigma = 0.16f,
            .drift_strength = 0.08f
        });
    AnimatedCurve& leader = m_particles.spawn(std::move(leader_builder));
    ++m_spawn.leader_count;

    for (int i = 0; i < 240; ++i) {
        m_sim_time += 1.f / 60.f;
        m_particles.update(1.f / 60.f, 1.f, m_sim_time);
    }

    ndde::SeekParticleBehavior::Params seek;
    seek.target = TargetSelector::nearest(ParticleRole::Leader);
    seek.speed = 0.95f;
    seek.delay_seconds = 1.0f;

    const glm::vec2 uv = offset_spawn(leader.head_uv(), 2.3f, 0.4f);
    ParticleBuilder seeker_builder = m_particles.factory().particle();
    seeker_builder
        .named("Chaser - Delayed Seek - Brownian")
        .role(ParticleRole::Chaser)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_behavior<SeekParticleBehavior>(seek)
        .with_behavior<BrownianBehavior>(0.25f, BrownianBehavior::Params{
            .sigma = 0.06f,
            .drift_strength = 0.f
        });
    m_particles.spawn(std::move(seeker_builder));
    ++m_spawn.chaser_count;
    ++m_spawn.delay_pursuit_count;

    m_particles.add_goal<CaptureGoal>(CaptureGoal::Params{
        .seeker_role = ParticleRole::Chaser,
        .target_role = ParticleRole::Leader,
        .radius = 0.22f
    });
    m_goal_status = GoalStatus::Running;
}

void SurfaceSimSpawner::spawn_leader_seeker() {
    const float u_mid = 0.5f*(m_surface->u_min() + m_surface->u_max());
    const float v_mid = 0.5f*(m_surface->v_min() + m_surface->v_max());

    std::unique_ptr<ndde::sim::IEquation> eq;
    if (m_pursuit.leader_mode == LeaderMode::Deterministic)
        eq = std::make_unique<ndde::sim::LeaderSeekerEquation>(
                &m_pursuit.extremum_table, m_pursuit.leader_seeker);
    else
        eq = std::make_unique<ndde::sim::BiasedBrownianLeader>(
                &m_pursuit.extremum_table, m_pursuit.biased_brownian_leader);

    const std::size_t cap =
        static_cast<std::size_t>(std::ceil(m_pursuit.pursuit_tau * 120.f * 1.5f)) + 256;

    auto builder = m_particles.factory().particle();
    builder
        .named(m_pursuit.leader_mode == LeaderMode::Deterministic
            ? "Leader - Extremum Seeker"
            : "Leader - Biased Brownian")
        .role(ParticleRole::Leader)
        .at({u_mid, v_mid})
        .history(cap, 1.f / 120.f)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_equation(std::move(eq));
    m_particles.spawn(std::move(builder));
    ++m_spawn.leader_count;
    m_spawn.spawning_pursuer = true;
}

void SurfaceSimSpawner::spawn_pursuit_particle() {
    if (m_curves.empty()) return;

    if (m_curves[0].history() == nullptr) {
        const std::size_t cap =
            static_cast<std::size_t>(std::ceil(m_pursuit.pursuit_tau * 120.f * 1.5f)) + 256;
        m_curves[0].enable_history(cap, 1.f / 120.f);
    }

    const glm::vec2 ref = m_curves[0].head_uv();
    const glm::vec2 uv  = offset_spawn(ref, 2.0f,
        static_cast<float>(m_spawn.delay_pursuit_count) * 1.1f + 0.3f);

    std::unique_ptr<ndde::sim::IEquation> eq;
    switch (m_pursuit.pursuit_mode) {
        case PursuitMode::Direct:
            eq = std::make_unique<ndde::sim::DirectPursuitEquation>(
                [this]{ return m_curves[0].head_uv(); },
                ndde::sim::DirectPursuitEquation::Params{
                    m_pursuit.leader_seeker.pursuit_speed, 0.f });
            break;
        case PursuitMode::Delayed:
            eq = std::make_unique<ndde::sim::DelayPursuitEquation>(
                m_curves[0].history(), m_surface.get(),
                ndde::sim::DelayPursuitEquation::Params{
                    m_pursuit.pursuit_tau, m_pursuit.leader_seeker.pursuit_speed, 0.f });
            break;
        case PursuitMode::Momentum:
            eq = std::make_unique<ndde::sim::MomentumBearingEquation>(
                m_curves[0].history(),
                ndde::sim::MomentumBearingEquation::Params{
                    m_pursuit.leader_seeker.pursuit_speed, m_pursuit.pursuit_window, 0.f });
            break;
    }

    auto builder = m_particles.factory().particle();
    builder
        .named("Chaser - Pursuit")
        .role(ParticleRole::Chaser)
        .at(uv)
        .trail({TrailMode::Finite, AnimatedCurve::MAX_TRAIL, 0.015f})
        .stochastic()
        .with_equation(std::move(eq));
    m_particles.spawn(std::move(builder));
    ++m_spawn.chaser_count;
    ++m_spawn.delay_pursuit_count;
}

} // namespace ndde
