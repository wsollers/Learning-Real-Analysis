#pragma once
// geometry/Parabola.hpp
// p(t) = (t, a·t² + b·t + c, 0)  in R^3
// Default: vertex at origin, opens upward, mapped to NDC via scale factor.

#include "Curve.hpp"

namespace ndde::geometry {

class Parabola final : public Curve {
public:
    /// y = a·t² + b·t + c, displayed on the XY plane (z=0).
    /// tmin/tmax define the visible parameter range.
    explicit Parabola(
        float a     = 1.f,
        float b     = 0.f,
        float c     = 0.f,
        float tmin  = -1.f,
        float tmax  =  1.f
    );

    [[nodiscard]] Vec3  evaluate(float t) const override;
    [[nodiscard]] float t_min()           const override { return m_tmin; }
    [[nodiscard]] float t_max()           const override { return m_tmax; }

    // Accessors for testing
    float a() const { return m_a; }
    float b() const { return m_b; }
    float c() const { return m_c; }

private:
    float m_a, m_b, m_c;
    float m_tmin, m_tmax;
};

} // namespace ndde::geometry
