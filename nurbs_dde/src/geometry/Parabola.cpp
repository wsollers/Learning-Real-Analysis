// geometry/Parabola.cpp
#include "Parabola.hpp"
#include <stdexcept>

namespace ndde::geometry {

Parabola::Parabola(float a, float b, float c, float tmin, float tmax)
    : m_a(a), m_b(b), m_c(c), m_tmin(tmin), m_tmax(tmax)
{
    if (tmin >= tmax) {
        throw std::invalid_argument("Parabola: tmin must be < tmax");
    }
}

Vec3 Parabola::evaluate(float t) const {
    // x = t (horizontal axis), y = a·t²+b·t+c (vertical), z = 0 (XY plane)
    return Vec3{
        t,
        m_a * t * t + m_b * t + m_c,
        0.f
    };
}

} // namespace ndde::geometry
