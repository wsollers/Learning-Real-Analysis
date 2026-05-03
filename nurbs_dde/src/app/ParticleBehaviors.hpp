#pragma once
// app/ParticleBehaviors.hpp
// Composable behavior stack for particle dynamics.

#include "app/SimulationContext.hpp"
#include "sim/IEquation.hpp"
#include "numeric/ops.hpp"
#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace ndde {

struct TargetSelector {
    enum class Kind : u8 { ById, FirstRole, NearestRole, CentroidRole };

    Kind kind = Kind::FirstRole;
    ParticleId id = 0;
    ParticleRole role = ParticleRole::Leader;

    [[nodiscard]] static TargetSelector by_id(ParticleId id) noexcept {
        TargetSelector s; s.kind = Kind::ById; s.id = id; return s;
    }
    [[nodiscard]] static TargetSelector first(ParticleRole role) noexcept {
        TargetSelector s; s.kind = Kind::FirstRole; s.role = role; return s;
    }
    [[nodiscard]] static TargetSelector nearest(ParticleRole role) noexcept {
        TargetSelector s; s.kind = Kind::NearestRole; s.role = role; return s;
    }
    [[nodiscard]] static TargetSelector centroid(ParticleRole role) noexcept {
        TargetSelector s; s.kind = Kind::CentroidRole; s.role = role; return s;
    }
};

class IParticleBehavior {
public:
    virtual ~IParticleBehavior() = default;

    [[nodiscard]] virtual glm::vec2 velocity(
        ndde::sim::ParticleState&    state,
        const ndde::math::ISurface&  surface,
        float                        t,
        const SimulationContext&     context,
        ParticleId                   owner) const = 0;

    [[nodiscard]] virtual glm::vec2 noise_coefficient(
        const ndde::sim::ParticleState& /*state*/,
        const ndde::math::ISurface&     /*surface*/,
        float                           /*t*/,
        const SimulationContext&        /*context*/,
        ParticleId                      /*owner*/) const
    {
        return {0.f, 0.f};
    }

    [[nodiscard]] virtual float phase_rate() const { return 0.f; }
    [[nodiscard]] virtual std::string metadata_label() const = 0;
};

class EquationBehavior final : public IParticleBehavior {
public:
    explicit EquationBehavior(std::unique_ptr<ndde::sim::IEquation> equation)
        : m_equation(std::move(equation))
    {}

    [[nodiscard]] glm::vec2 velocity(ndde::sim::ParticleState& state,
                                     const ndde::math::ISurface& surface,
                                     float t,
                                     const SimulationContext&,
                                     ParticleId) const override {
        return m_equation ? m_equation->update(state, surface, t) : glm::vec2{0.f, 0.f};
    }

    [[nodiscard]] glm::vec2 noise_coefficient(const ndde::sim::ParticleState& state,
                                              const ndde::math::ISurface& surface,
                                              float t,
                                              const SimulationContext&,
                                              ParticleId) const override {
        return m_equation ? m_equation->noise_coefficient(state, surface, t) : glm::vec2{0.f, 0.f};
    }

    [[nodiscard]] float phase_rate() const override {
        return m_equation ? m_equation->phase_rate() : 0.f;
    }

    [[nodiscard]] std::string metadata_label() const override {
        return m_equation ? m_equation->name() : "Equation";
    }

    [[nodiscard]] ndde::sim::IEquation* equation() noexcept { return m_equation.get(); }
    [[nodiscard]] const ndde::sim::IEquation* equation() const noexcept { return m_equation.get(); }

private:
    std::unique_ptr<ndde::sim::IEquation> m_equation;
};

class BrownianBehavior final : public IParticleBehavior {
public:
    struct Params {
        float sigma = 0.4f;
        float drift_strength = 0.f;
    };

    explicit BrownianBehavior(Params p = {}) : m_p(p) {}

    [[nodiscard]] glm::vec2 velocity(ndde::sim::ParticleState& state,
                                     const ndde::math::ISurface& surface,
                                     float,
                                     const SimulationContext&,
                                     ParticleId) const override {
        if (ops::abs(m_p.drift_strength) < 1e-7f) return {0.f, 0.f};
        const glm::vec3 du = surface.du(state.uv.x, state.uv.y);
        const glm::vec3 dv = surface.dv(state.uv.x, state.uv.y);
        const float gn = ops::sqrt(du.z * du.z + dv.z * dv.z) + 1e-7f;
        return {m_p.drift_strength * du.z / gn, m_p.drift_strength * dv.z / gn};
    }

    [[nodiscard]] glm::vec2 noise_coefficient(const ndde::sim::ParticleState&,
                                              const ndde::math::ISurface&,
                                              float,
                                              const SimulationContext&,
                                              ParticleId) const override {
        return {m_p.sigma, m_p.sigma};
    }

    [[nodiscard]] std::string metadata_label() const override {
        return ops::abs(m_p.drift_strength) > 1e-7f ? "Brownian + Bias Drift" : "Brownian";
    }

private:
    Params m_p;
};

class ConstantDriftBehavior final : public IParticleBehavior {
public:
    explicit ConstantDriftBehavior(glm::vec2 velocity) : m_velocity(velocity) {}

    [[nodiscard]] glm::vec2 velocity(ndde::sim::ParticleState&,
                                     const ndde::math::ISurface&,
                                     float,
                                     const SimulationContext&,
                                     ParticleId) const override {
        return m_velocity;
    }

    [[nodiscard]] std::string metadata_label() const override {
        return "Constant Drift";
    }

private:
    glm::vec2 m_velocity{0.f, 0.f};
};

class SeekParticleBehavior final : public IParticleBehavior {
public:
    struct Params {
        TargetSelector target = TargetSelector::first(ParticleRole::Leader);
        float speed = 0.8f;
        float delay_seconds = 0.f;
    };

    explicit SeekParticleBehavior(Params p = {}) : m_p(p) {}

    [[nodiscard]] glm::vec2 velocity(ndde::sim::ParticleState& state,
                                     const ndde::math::ISurface& surface,
                                     float t,
                                     const SimulationContext& context,
                                     ParticleId owner) const override {
        return direction_to_target(state.uv, surface, t, context, owner) * m_p.speed;
    }

    [[nodiscard]] std::string metadata_label() const override {
        return m_p.delay_seconds > 0.f ? "Delayed Seek" : "Seek";
    }

private:
    Params m_p;

    [[nodiscard]] glm::vec2 direction_to_target(glm::vec2 from,
                                                const ndde::math::ISurface& surface,
                                                float t,
                                                const SimulationContext& context,
                                                ParticleId owner) const;
};

class AvoidParticleBehavior final : public IParticleBehavior {
public:
    struct Params {
        TargetSelector target = TargetSelector::nearest(ParticleRole::Chaser);
        float speed = 0.8f;
        float delay_seconds = 0.f;
    };

    explicit AvoidParticleBehavior(Params p = {}) : m_p(p) {}

    [[nodiscard]] glm::vec2 velocity(ndde::sim::ParticleState& state,
                                     const ndde::math::ISurface& surface,
                                     float t,
                                     const SimulationContext& context,
                                     ParticleId owner) const override;

    [[nodiscard]] std::string metadata_label() const override {
        return m_p.delay_seconds > 0.f ? "Delayed Avoid" : "Avoid";
    }

private:
    SeekParticleBehavior::Params seek_params() const {
        return {.target = m_p.target, .speed = 1.f, .delay_seconds = m_p.delay_seconds};
    }
    Params m_p;
};

class CentroidSeekBehavior final : public IParticleBehavior {
public:
    struct Params {
        ParticleRole role = ParticleRole::Chaser;
        float speed = 0.8f;
    };

    explicit CentroidSeekBehavior(Params p = {}) : m_p(p) {}

    [[nodiscard]] glm::vec2 velocity(ndde::sim::ParticleState& state,
                                     const ndde::math::ISurface& surface,
                                     float,
                                     const SimulationContext& context,
                                     ParticleId owner) const override;

    [[nodiscard]] std::string metadata_label() const override {
        return "Centroid Seek";
    }

private:
    Params m_p;
};

class GradientDriftBehavior final : public IParticleBehavior {
public:
    enum class Mode : u8 {
        Uphill,
        Downhill,
        LevelTangent
    };

    struct Params {
        Mode mode = Mode::Uphill;
        float speed = 0.6f;
    };

    explicit GradientDriftBehavior(Params p = {}) : m_p(p) {}

    [[nodiscard]] glm::vec2 velocity(ndde::sim::ParticleState& state,
                                     const ndde::math::ISurface& surface,
                                     float,
                                     const SimulationContext&,
                                     ParticleId) const override;

    [[nodiscard]] std::string metadata_label() const override;

private:
    Params m_p;
};

class OrbitBehavior final : public IParticleBehavior {
public:
    struct Params {
        glm::vec2 center{0.f, 0.f};
        float radius = 1.f;
        float angular_speed = 0.7f;
        float radial_strength = 0.45f;
        bool clockwise = false;
    };

    explicit OrbitBehavior(Params p = {}) : m_p(p) {}

    [[nodiscard]] glm::vec2 velocity(ndde::sim::ParticleState& state,
                                     const ndde::math::ISurface& surface,
                                     float,
                                     const SimulationContext&,
                                     ParticleId) const override;

    [[nodiscard]] std::string metadata_label() const override { return "Orbit"; }

private:
    Params m_p;
};

class FlockingBehavior final : public IParticleBehavior {
public:
    struct Params {
        ParticleRole role = ParticleRole::Neutral;
        float neighbor_radius = 1.2f;
        float separation_radius = 0.35f;
        float separation_weight = 1.0f;
        float cohesion_weight = 0.45f;
        float alignment_weight = 0.35f;
        float speed = 0.65f;
    };

    explicit FlockingBehavior(Params p = {}) : m_p(p) {}

    [[nodiscard]] glm::vec2 velocity(ndde::sim::ParticleState& state,
                                     const ndde::math::ISurface& surface,
                                     float,
                                     const SimulationContext& context,
                                     ParticleId owner) const override;

    [[nodiscard]] std::string metadata_label() const override { return "Flocking"; }

private:
    Params m_p;
};

struct SlopeVelocityTransform {
    bool enabled = false;
    float intercept = 1.f;
    float slope_gain = 0.f;
    float min_scale = 0.f;
    float max_scale = std::numeric_limits<float>::infinity();

    [[nodiscard]] static SlopeVelocityTransform identity() noexcept { return {}; }
};

class BehaviorStack final : public ndde::sim::IEquation {
public:
    BehaviorStack() = default;
    BehaviorStack(const BehaviorStack&) = delete;
    BehaviorStack& operator=(const BehaviorStack&) = delete;
    BehaviorStack(BehaviorStack&&) noexcept = default;
    BehaviorStack& operator=(BehaviorStack&&) noexcept = default;

    void set_context(const SimulationContext* context) noexcept { m_context = context; }
    void set_owner(ParticleId owner) noexcept { m_owner = owner; }

    void add(std::unique_ptr<IParticleBehavior> behavior, float weight = 1.f) {
        m_behaviors.push_back({std::move(behavior), weight});
    }

    void set_velocity_transform(SlopeVelocityTransform transform) noexcept {
        m_velocity_transform = transform;
    }

    [[nodiscard]] glm::vec2 update(ndde::sim::ParticleState& state,
                                   const ndde::math::ISurface& surface,
                                   float t) const override {
        if (!m_context) return {0.f, 0.f};
        glm::vec2 sum{0.f, 0.f};
        for (const auto& entry : m_behaviors)
            sum += entry.weight * entry.behavior->velocity(state, surface, t, *m_context, m_owner);
        return apply_velocity_transform(sum, state, surface);
    }

    [[nodiscard]] glm::vec2 noise_coefficient(const ndde::sim::ParticleState& state,
                                              const ndde::math::ISurface& surface,
                                              float t) const override {
        if (!m_context) return {0.f, 0.f};
        glm::vec2 sum{0.f, 0.f};
        for (const auto& entry : m_behaviors)
            sum += entry.weight * entry.behavior->noise_coefficient(state, surface, t, *m_context, m_owner);
        return sum;
    }

    [[nodiscard]] float phase_rate() const override {
        float rate = 0.f;
        for (const auto& entry : m_behaviors)
            rate += entry.weight * entry.behavior->phase_rate();
        return rate;
    }

    [[nodiscard]] std::string name() const override;
    [[nodiscard]] std::vector<std::string> behavior_labels() const;

    template <class Equation>
    [[nodiscard]] Equation* find_equation() noexcept {
        for (const auto& entry : m_behaviors) {
            auto* wrapper = dynamic_cast<EquationBehavior*>(entry.behavior.get());
            if (!wrapper) continue;
            if (auto* eq = dynamic_cast<Equation*>(wrapper->equation()))
                return eq;
        }
        return nullptr;
    }

    template <class Equation>
    [[nodiscard]] const Equation* find_equation() const noexcept {
        for (const auto& entry : m_behaviors) {
            const auto* wrapper = dynamic_cast<const EquationBehavior*>(entry.behavior.get());
            if (!wrapper) continue;
            if (const auto* eq = dynamic_cast<const Equation*>(wrapper->equation()))
                return eq;
        }
        return nullptr;
    }

private:
    struct Entry {
        std::unique_ptr<IParticleBehavior> behavior;
        float weight = 1.f;
    };

    std::vector<Entry> m_behaviors;
    const SimulationContext* m_context = nullptr;
    ParticleId m_owner = 0;
    SlopeVelocityTransform m_velocity_transform{};

    [[nodiscard]] glm::vec2 apply_velocity_transform(glm::vec2 velocity,
                                                     const ndde::sim::ParticleState& state,
                                                     const ndde::math::ISurface& surface) const noexcept {
        if (!m_velocity_transform.enabled) return velocity;
        const float speed = ops::length(velocity);
        if (speed < 1e-7f) return velocity;

        const glm::vec2 dir = velocity / speed;
        const glm::vec3 du = surface.du(state.uv.x, state.uv.y);
        const glm::vec3 dv = surface.dv(state.uv.x, state.uv.y);
        const float directional_derivative = du.z * dir.x + dv.z * dir.y;
        const float scale = ops::clamp(
            m_velocity_transform.intercept + m_velocity_transform.slope_gain * directional_derivative,
            m_velocity_transform.min_scale,
            m_velocity_transform.max_scale);
        return velocity * scale;
    }
};

} // namespace ndde
