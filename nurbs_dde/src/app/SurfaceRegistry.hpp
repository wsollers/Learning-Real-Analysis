#pragma once
// app/SurfaceRegistry.hpp
// Named surface definitions used by simulation scenes.

#include "math/SineRationalSurface.hpp"
#include "math/Surfaces.hpp"
#include "numeric/ops.hpp"

#include <memory>
#include <string_view>

namespace ndde {

class MultiWellWaveSurface final : public ndde::math::ISurface {
public:
    explicit MultiWellWaveSurface(float extent = 4.f) : m_extent(extent) {}

    [[nodiscard]] Vec3 evaluate(float x, float y, float = 0.f) const override {
        return {x, y, height(x, y)};
    }

    [[nodiscard]] float height(float x, float y) const noexcept {
        const float g0 = 1.15f * ops::exp(-((x - 1.4f) * (x - 1.4f) + (y - 0.6f) * (y - 0.6f)));
        const float g1 = -0.85f * ops::exp(-(((x + 1.2f) * (x + 1.2f)) / 0.8f
                                            + ((y + 1.0f) * (y + 1.0f)) / 1.4f));
        const float g2 = 0.65f * ops::exp(-(((x - 0.2f) * (x - 0.2f)) / 1.8f
                                           + ((y - 1.6f) * (y - 1.6f)) / 0.6f));
        const float w0 = 0.18f * ops::sin(2.f * x) * ops::cos(2.f * y);
        const float w1 = 0.08f * ops::sin(4.f * x + y) * ops::cos(3.f * y - x);
        return g0 + g1 + g2 + w0 + w1;
    }

    [[nodiscard]] float u_min(float = 0.f) const override { return -m_extent; }
    [[nodiscard]] float u_max(float = 0.f) const override { return  m_extent; }
    [[nodiscard]] float v_min(float = 0.f) const override { return -m_extent; }
    [[nodiscard]] float v_max(float = 0.f) const override { return  m_extent; }
    [[nodiscard]] float extent() const noexcept { return m_extent; }

private:
    float m_extent = 4.f;
};

class WavePredatorPreySurface final : public ndde::math::ISurface {
public:
    explicit WavePredatorPreySurface(float extent = 4.f) : m_extent(extent) {}

    [[nodiscard]] Vec3 evaluate(float x, float y, float = 0.f) const override {
        return {x, y, height(x, y)};
    }

    [[nodiscard]] float height(float x, float y) const noexcept {
        return ops::sin(x) + ops::cos(y)
             + 0.5f * (ops::sin(2.f * x) + ops::cos(2.f * y));
    }

    [[nodiscard]] float u_min(float = 0.f) const override { return -m_extent; }
    [[nodiscard]] float u_max(float = 0.f) const override { return  m_extent; }
    [[nodiscard]] float v_min(float = 0.f) const override { return -m_extent; }
    [[nodiscard]] float v_max(float = 0.f) const override { return  m_extent; }
    [[nodiscard]] float extent() const noexcept { return m_extent; }

private:
    float m_extent = 4.f;
};

enum class SurfaceKey : u8 {
    SineRational,
    MultiWell,
    WavePredatorPrey
};

struct SurfaceDescriptor {
    SurfaceKey key = SurfaceKey::SineRational;
    std::string_view id;
    std::string_view display_name;
    std::string_view formula;
};

class SurfaceRegistry {
public:
    [[nodiscard]] static SurfaceDescriptor describe(SurfaceKey key) noexcept {
        switch (key) {
            case SurfaceKey::MultiWell:
                return {key, "multi-well", "Multi-Well",
                    "z = Gaussian wells + 0.18 sin(2x)cos(2y) + 0.08 sin(4x+y)cos(3y-x)"};
            case SurfaceKey::WavePredatorPrey:
                return {key, "wave-predator-prey", "Wave Predator-Prey",
                    "z = sin x + cos y + 0.5(sin 2x + cos 2y)"};
            case SurfaceKey::SineRational:
            default:
                return {key, "sine-rational", "Sine-Rational",
                    "z = [3/(1+(x+y+1)^2)] sin(2x)cos(2y) + 0.1 sin(5x)sin(5y)"};
        }
    }

    [[nodiscard]] static std::unique_ptr<ndde::math::SineRationalSurface> make_sine_rational(float extent = 4.f) {
        return std::make_unique<ndde::math::SineRationalSurface>(extent);
    }

    [[nodiscard]] static std::unique_ptr<MultiWellWaveSurface> make_multi_well(float extent = 4.f) {
        return std::make_unique<MultiWellWaveSurface>(extent);
    }

    [[nodiscard]] static std::unique_ptr<WavePredatorPreySurface> make_wave_predator_prey(float extent = 4.f) {
        return std::make_unique<WavePredatorPreySurface>(extent);
    }
};

} // namespace ndde
