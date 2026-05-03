#pragma once
// app/ParticleSystem.hpp
// Scene-local owner for particles, particle constraints, and particle goals.

#include "app/AnimatedCurve.hpp"
#include "app/ParticleFactory.hpp"
#include "app/ParticleGoals.hpp"
#include "app/SimulationContext.hpp"
#include "engine/IScene.hpp"
#include "sim/EulerIntegrator.hpp"
#include "sim/IConstraint.hpp"
#include "sim/MilsteinIntegrator.hpp"
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace ndde {

class ParticleSystem {
public:
    explicit ParticleSystem(const ndde::math::ISurface* surface,
                            std::uint32_t seed = std::random_device{}())
        : m_surface(surface), m_rng(seed)
    {}

    void set_surface(const ndde::math::ISurface* surface) noexcept { m_surface = surface; }
    [[nodiscard]] const ndde::math::ISurface* surface() const noexcept { return m_surface; }

    [[nodiscard]] ParticleFactory factory() const noexcept { return ParticleFactory(m_surface); }
    [[nodiscard]] std::mt19937& rng() noexcept { return m_rng; }

    [[nodiscard]] std::vector<Particle>& particles() noexcept { return m_particles; }
    [[nodiscard]] const std::vector<Particle>& particles() const noexcept { return m_particles; }
    [[nodiscard]] bool empty() const noexcept { return m_particles.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return m_particles.size(); }

    [[nodiscard]] std::vector<ParticleSnapshot> snapshot_particles() const {
        std::vector<ParticleSnapshot> out;
        out.reserve(m_particles.size());
        for (const auto& particle : m_particles) {
            const glm::vec2 uv = particle.head_uv();
            const Vec3 head = particle.head_world();
            out.push_back(ParticleSnapshot{
                .id = particle.id(),
                .role = std::string(role_name(particle.particle_role())),
                .label = particle.metadata_label(),
                .u = uv.x,
                .v = uv.y,
                .x = head.x,
                .y = head.y,
                .z = head.z
            });
        }
        return out;
    }

    Particle& spawn(ParticleBuilder builder) {
        m_particles.push_back(builder.build(&m_euler, &m_milstein));
        return m_particles.back();
    }

    void clear() { m_particles.clear(); }
    void clear_goals() { m_goals.clear(); }
    void clear_pair_constraints() { m_pair_constraints.clear(); }
    void clear_all() {
        m_particles.clear();
        m_pair_constraints.clear();
        m_goals.clear();
    }

    void update(float dt, float speed_scale, float sim_time) {
        SimulationContext context(m_surface, &m_particles, &m_rng);
        context.set_time(sim_time);
        for (auto& particle : m_particles) {
            particle.set_behavior_context(&context);
            particle.advance(dt, speed_scale);
            particle.push_history(sim_time);
        }
        apply_pair_constraints();
    }

    [[nodiscard]] SimulationContext context(float sim_time) {
        SimulationContext c(m_surface, &m_particles, &m_rng);
        c.set_time(sim_time);
        return c;
    }

    template <class Constraint, class... Args>
    Constraint& add_pair_constraint(Args&&... args) {
        auto c = std::make_unique<Constraint>(std::forward<Args>(args)...);
        Constraint& ref = *c;
        m_pair_constraints.push_back(std::move(c));
        return ref;
    }

    template <class Goal, class... Args>
    Goal& add_goal(Args&&... args) {
        auto g = std::make_unique<Goal>(std::forward<Args>(args)...);
        Goal& ref = *g;
        m_goals.push_back(std::move(g));
        return ref;
    }

    [[nodiscard]] GoalStatus evaluate_goals(float sim_time) {
        SimulationContext c = context(sim_time);
        GoalStatus aggregate = GoalStatus::Running;
        for (const auto& goal : m_goals) {
            if (!goal) continue;
            const GoalStatus status = goal->evaluate(c);
            if (status != GoalStatus::Running)
                aggregate = status;
        }
        return aggregate;
    }

private:
    const ndde::math::ISurface* m_surface = nullptr;
    ndde::sim::EulerIntegrator m_euler;
    ndde::sim::MilsteinIntegrator m_milstein;
    std::vector<Particle> m_particles;
    std::vector<std::unique_ptr<ndde::sim::IPairConstraint>> m_pair_constraints;
    std::vector<std::unique_ptr<IParticleGoal>> m_goals;
    std::mt19937 m_rng;

    void apply_pair_constraints() {
        if (!m_surface || m_pair_constraints.empty()) return;
        for (auto& constraint : m_pair_constraints) {
            if (!constraint) continue;
            for (std::size_t i = 0; i < m_particles.size(); ++i) {
                for (std::size_t j = i + 1; j < m_particles.size(); ++j) {
                    constraint->apply(m_particles[i].walk_state(),
                                      m_particles[j].walk_state(),
                                      *m_surface);
                }
            }
        }
    }
};

} // namespace ndde
