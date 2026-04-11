#pragma once
// geometry/Curve.hpp
// Abstract parametric curve: p : [t0, t1] → R^3
// Concrete subclasses: Parabola, NURBS, DDE trajectory, …

#include "math/Vec3.hpp"
#include <vector>
#include <span>
#include <cstdint>

namespace ndde::geometry {

using math::Vec3;

// ── Parametric sample ─────────────────────────────────────────────────────────
struct Sample {
    float t;      ///< Parameter value
    Vec3  point;  ///< Position in R^3
};

// ── Abstract base ─────────────────────────────────────────────────────────────
class Curve {
public:
    virtual ~Curve() = default;

    /// Evaluate the curve at parameter t.
    [[nodiscard]] virtual Vec3 evaluate(float t) const = 0;

    /// Parameter domain [t0, t1].
    [[nodiscard]] virtual float t_min() const = 0;
    [[nodiscard]] virtual float t_max() const = 0;

    /// Uniform tessellation into n+1 samples.
    [[nodiscard]] std::vector<Sample> tessellate(std::uint32_t n) const;

    /// Flat vertex buffer suitable for a Vulkan line-strip.
    /// Returns interleaved (x,y,z,1) vec4 in NDC — caller maps to clip space.
    [[nodiscard]] std::vector<math::Vec4> vertex_buffer(std::uint32_t n) const;
};

} // namespace ndde::geometry
