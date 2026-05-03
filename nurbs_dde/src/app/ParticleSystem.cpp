#include "app/ParticleFactory.hpp"
#include "app/SimulationContext.hpp"
#include "app/ParticleBehaviors.hpp"
#include "app/AnimatedCurve.hpp"

#include <limits>

namespace ndde {

namespace {

[[nodiscard]] glm::vec2 shortest_delta(glm::vec2 target,
                                       glm::vec2 from,
                                       const ndde::math::ISurface& surface) noexcept {
    glm::vec2 delta = target - from;
    if (surface.is_periodic_u()) {
        const float span = surface.u_max() - surface.u_min();
        if (delta.x >  span * 0.5f) delta.x -= span;
        if (delta.x < -span * 0.5f) delta.x += span;
    }
    if (surface.is_periodic_v()) {
        const float span = surface.v_max() - surface.v_min();
        if (delta.y >  span * 0.5f) delta.y -= span;
        if (delta.y < -span * 0.5f) delta.y += span;
    }
    return delta;
}

[[nodiscard]] glm::vec2 normalize_or_zero(glm::vec2 v) noexcept {
    const float d = ops::length(v);
    return d > 1e-7f ? v / d : glm::vec2{0.f, 0.f};
}

[[nodiscard]] glm::vec2 target_position(const TargetSelector& selector,
                                        glm::vec2 from,
                                        const ndde::math::ISurface& surface,
                                        float t,
                                        const SimulationContext& context,
                                        ParticleId owner,
                                        float delay) noexcept {
    const AnimatedCurve* target = nullptr;
    switch (selector.kind) {
        case TargetSelector::Kind::ById:
            target = context.find(selector.id);
            break;
        case TargetSelector::Kind::FirstRole:
            target = context.first(selector.role, owner);
            break;
        case TargetSelector::Kind::NearestRole:
            target = context.nearest(selector.role, from, owner);
            break;
        case TargetSelector::Kind::CentroidRole:
            return context.centroid(selector.role, owner);
    }

    if (!target) return from;
    if (delay > 0.f) {
        if (const auto* history = target->history())
            return history->query(t - delay);
    }
    (void)surface;
    return target->head_uv();
}

} // namespace

const AnimatedCurve* SimulationContext::find(ParticleId id) const noexcept {
    if (!m_particles) return nullptr;
    for (const auto& particle : *m_particles) {
        if (particle.id() == id) return &particle;
    }
    return nullptr;
}

const AnimatedCurve* SimulationContext::first(ParticleRole role, ParticleId exclude) const noexcept {
    if (!m_particles) return nullptr;
    for (const auto& particle : *m_particles) {
        if (particle.id() != exclude && particle.particle_role() == role)
            return &particle;
    }
    return nullptr;
}

const AnimatedCurve* SimulationContext::nearest(ParticleRole role, glm::vec2 from, ParticleId exclude) const noexcept {
    if (!m_particles || !m_surface) return nullptr;
    const AnimatedCurve* best = nullptr;
    float best_d2 = std::numeric_limits<float>::max();
    for (const auto& particle : *m_particles) {
        if (particle.id() == exclude || particle.particle_role() != role) continue;
        const glm::vec2 d = shortest_delta(particle.head_uv(), from, *m_surface);
        const float d2 = d.x * d.x + d.y * d.y;
        if (d2 < best_d2) {
            best_d2 = d2;
            best = &particle;
        }
    }
    return best;
}

glm::vec2 SimulationContext::centroid(ParticleRole role, ParticleId exclude) const noexcept {
    if (!m_particles) return {0.f, 0.f};
    glm::vec2 sum{0.f, 0.f};
    u32 count = 0;
    for (const auto& particle : *m_particles) {
        if (particle.id() == exclude || particle.particle_role() != role) continue;
        sum += particle.head_uv();
        ++count;
    }
    return count > 0 ? sum / static_cast<float>(count) : glm::vec2{0.f, 0.f};
}

glm::vec2 SeekParticleBehavior::direction_to_target(glm::vec2 from,
                                                    const ndde::math::ISurface& surface,
                                                    float t,
                                                    const SimulationContext& context,
                                                    ParticleId owner) const {
    const glm::vec2 target = target_position(m_p.target, from, surface, t, context, owner, m_p.delay_seconds);
    return normalize_or_zero(shortest_delta(target, from, surface));
}

glm::vec2 AvoidParticleBehavior::velocity(ndde::sim::ParticleState& state,
                                          const ndde::math::ISurface& surface,
                                          float t,
                                          const SimulationContext& context,
                                          ParticleId owner) const {
    const glm::vec2 target = target_position(m_p.target, state.uv, surface, t, context, owner, m_p.delay_seconds);
    return -normalize_or_zero(shortest_delta(target, state.uv, surface)) * m_p.speed;
}

glm::vec2 CentroidSeekBehavior::velocity(ndde::sim::ParticleState& state,
                                         const ndde::math::ISurface& surface,
                                         float,
                                         const SimulationContext& context,
                                         ParticleId owner) const {
    const glm::vec2 target = context.centroid(m_p.role, owner);
    return normalize_or_zero(shortest_delta(target, state.uv, surface)) * m_p.speed;
}

std::string BehaviorStack::name() const {
    if (m_behaviors.empty()) return "BehaviorStack";
    std::string out;
    for (const auto& entry : m_behaviors) {
        if (!out.empty()) out += " + ";
        out += entry.behavior->metadata_label();
    }
    return out;
}

std::vector<std::string> BehaviorStack::behavior_labels() const {
    std::vector<std::string> labels;
    labels.reserve(m_behaviors.size());
    for (const auto& entry : m_behaviors)
        labels.push_back(entry.behavior->metadata_label());
    return labels;
}

AnimatedCurve ParticleBuilder::build(const ndde::sim::IIntegrator* deterministic,
                                     const ndde::sim::IIntegrator* stochastic) {
    const ndde::sim::IIntegrator* integrator = (m_stochastic && stochastic) ? stochastic : deterministic;
    auto stack = std::make_unique<BehaviorStack>(std::move(m_stack));

    AnimatedCurve particle = AnimatedCurve::with_equation(
        m_uv.x, m_uv.y,
        m_role == ParticleRole::Chaser ? AnimatedCurve::Role::Chaser : AnimatedCurve::Role::Leader,
        0u,
        m_surface,
        std::move(stack),
        integrator);

    particle.set_particle_role(m_role);
    particle.set_label(m_label);
    particle.set_trail_config(m_trail);
    if (m_enable_history)
        particle.enable_history(m_history_capacity, m_history_dt_min);
    for (auto& constraint : m_constraints)
        particle.add_constraint(std::move(constraint));
    particle.bind_behavior_stack();
    return particle;
}

} // namespace ndde
