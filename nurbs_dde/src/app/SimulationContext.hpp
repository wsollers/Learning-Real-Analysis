#pragma once
// app/SimulationContext.hpp
// Lightweight per-scene context passed to composable particle behaviors.

#include "app/ParticleTypes.hpp"
#include "math/Surfaces.hpp"
#include "sim/HistoryBuffer.hpp"
#include <glm/glm.hpp>
#include <random>
#include <vector>

namespace ndde {

class AnimatedCurve;

class SimulationContext {
public:
    SimulationContext(const ndde::math::ISurface* surface,
                      const std::vector<AnimatedCurve>* particles,
                      std::mt19937* rng) noexcept
        : m_surface(surface), m_particles(particles), m_rng(rng)
    {}

    void set_time(float t) noexcept { m_time = t; }
    [[nodiscard]] float time() const noexcept { return m_time; }

    [[nodiscard]] const ndde::math::ISurface& surface() const noexcept { return *m_surface; }
    [[nodiscard]] std::mt19937& rng() const noexcept { return *m_rng; }

    [[nodiscard]] const std::vector<AnimatedCurve>& particles() const noexcept {
        return *m_particles;
    }

    [[nodiscard]] const AnimatedCurve* find(ParticleId id) const noexcept;
    [[nodiscard]] const AnimatedCurve* first(ParticleRole role, ParticleId exclude = 0) const noexcept;
    [[nodiscard]] const AnimatedCurve* nearest(ParticleRole role, glm::vec2 from, ParticleId exclude = 0) const noexcept;
    [[nodiscard]] glm::vec2 centroid(ParticleRole role, ParticleId exclude = 0) const noexcept;

private:
    const ndde::math::ISurface* m_surface = nullptr;
    const std::vector<AnimatedCurve>* m_particles = nullptr;
    std::mt19937* m_rng = nullptr;
    float m_time = 0.f;
};

} // namespace ndde
