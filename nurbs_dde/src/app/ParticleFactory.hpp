#pragma once
// app/ParticleFactory.hpp
// Fluent builder for composable particles.

#include "app/AnimatedCurve.hpp"
#include "app/ParticleBehaviors.hpp"
#include "sim/DomainConfinement.hpp"
#include "sim/EulerIntegrator.hpp"
#include "sim/IConstraint.hpp"
#include "sim/MilsteinIntegrator.hpp"
#include <memory>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace ndde {

class ParticleBuilder {
public:
    explicit ParticleBuilder(const ndde::math::ISurface* surface) : m_surface(surface) {}
    ParticleBuilder(const ParticleBuilder&) = delete;
    ParticleBuilder& operator=(const ParticleBuilder&) = delete;
    ParticleBuilder(ParticleBuilder&&) noexcept = default;
    ParticleBuilder& operator=(ParticleBuilder&&) noexcept = default;

    ParticleBuilder& named(std::string label) & {
        m_label = std::move(label);
        return *this;
    }
    ParticleBuilder&& named(std::string label) && {
        named(std::move(label));
        return std::move(*this);
    }

    ParticleBuilder& role(ParticleRole role) & noexcept {
        m_role = role;
        return *this;
    }
    ParticleBuilder&& role(ParticleRole role) && noexcept {
        this->role(role);
        return std::move(*this);
    }

    ParticleBuilder& at(glm::vec2 uv) & noexcept {
        m_uv = uv;
        return *this;
    }
    ParticleBuilder&& at(glm::vec2 uv) && noexcept {
        this->at(uv);
        return std::move(*this);
    }

    ParticleBuilder& trail(TrailConfig cfg) & noexcept {
        m_trail = cfg;
        return *this;
    }
    ParticleBuilder&& trail(TrailConfig cfg) && noexcept {
        this->trail(cfg);
        return std::move(*this);
    }

    ParticleBuilder& history(std::size_t capacity = 4096, float dt_min = 1.f / 120.f) & noexcept {
        m_history_capacity = capacity;
        m_history_dt_min = dt_min;
        m_enable_history = true;
        return *this;
    }
    ParticleBuilder&& history(std::size_t capacity = 4096, float dt_min = 1.f / 120.f) && noexcept {
        this->history(capacity, dt_min);
        return std::move(*this);
    }

    ParticleBuilder& stochastic(bool enabled = true) & noexcept {
        m_stochastic = enabled;
        return *this;
    }
    ParticleBuilder&& stochastic(bool enabled = true) && noexcept {
        this->stochastic(enabled);
        return std::move(*this);
    }

    ParticleBuilder& slope_velocity_transform(SlopeVelocityTransform transform) & noexcept {
        transform.enabled = true;
        m_stack.set_velocity_transform(transform);
        return *this;
    }
    ParticleBuilder&& slope_velocity_transform(SlopeVelocityTransform transform) && noexcept {
        this->slope_velocity_transform(transform);
        return std::move(*this);
    }

    ParticleBuilder& slope_velocity_transform(float intercept,
                                              float slope_gain,
                                              float min_scale = 0.f,
                                              float max_scale = std::numeric_limits<float>::infinity()) & noexcept {
        return slope_velocity_transform(SlopeVelocityTransform{
            .enabled = true,
            .intercept = intercept,
            .slope_gain = slope_gain,
            .min_scale = min_scale,
            .max_scale = max_scale
        });
    }
    ParticleBuilder&& slope_velocity_transform(float intercept,
                                               float slope_gain,
                                               float min_scale = 0.f,
                                               float max_scale = std::numeric_limits<float>::infinity()) && noexcept {
        this->slope_velocity_transform(intercept, slope_gain, min_scale, max_scale);
        return std::move(*this);
    }

    template <class Behavior, class... Args>
    ParticleBuilder& with_behavior(float weight, Args&&... args) & {
        m_stack.add(std::make_unique<Behavior>(std::forward<Args>(args)...), weight);
        return *this;
    }
    template <class Behavior, class... Args>
    ParticleBuilder&& with_behavior(float weight, Args&&... args) && {
        this->with_behavior<Behavior>(weight, std::forward<Args>(args)...);
        return std::move(*this);
    }

    template <class Behavior, class... Args>
    ParticleBuilder& with_behavior(Args&&... args) & {
        return with_behavior<Behavior>(1.f, std::forward<Args>(args)...);
    }
    template <class Behavior, class... Args>
    ParticleBuilder&& with_behavior(Args&&... args) && {
        this->with_behavior<Behavior>(1.f, std::forward<Args>(args)...);
        return std::move(*this);
    }

    ParticleBuilder& with_equation(std::unique_ptr<ndde::sim::IEquation> equation, float weight = 1.f) & {
        m_stack.add(std::make_unique<EquationBehavior>(std::move(equation)), weight);
        return *this;
    }
    ParticleBuilder&& with_equation(std::unique_ptr<ndde::sim::IEquation> equation, float weight = 1.f) && {
        this->with_equation(std::move(equation), weight);
        return std::move(*this);
    }

    template <class Constraint, class... Args>
    ParticleBuilder& with_constraint(Args&&... args) & {
        m_constraints.push_back(std::make_unique<Constraint>(std::forward<Args>(args)...));
        return *this;
    }
    template <class Constraint, class... Args>
    ParticleBuilder&& with_constraint(Args&&... args) && {
        this->with_constraint<Constraint>(std::forward<Args>(args)...);
        return std::move(*this);
    }

    [[nodiscard]] AnimatedCurve build(const ndde::sim::IIntegrator* deterministic,
                                      const ndde::sim::IIntegrator* stochastic = nullptr);

private:
    const ndde::math::ISurface* m_surface = nullptr;
    glm::vec2 m_uv{0.f, 0.f};
    ParticleRole m_role = ParticleRole::Neutral;
    std::string m_label;
    TrailConfig m_trail{};
    bool m_enable_history = false;
    std::size_t m_history_capacity = 4096;
    float m_history_dt_min = 1.f / 120.f;
    bool m_stochastic = false;
    BehaviorStack m_stack;
    std::vector<std::unique_ptr<ndde::sim::IConstraint>> m_constraints;
};

class ParticleFactory {
public:
    explicit ParticleFactory(const ndde::math::ISurface* surface) : m_surface(surface) {}

    [[nodiscard]] ParticleBuilder particle() const { return ParticleBuilder(m_surface); }

private:
    const ndde::math::ISurface* m_surface = nullptr;
};

using Particle = AnimatedCurve;

} // namespace ndde
