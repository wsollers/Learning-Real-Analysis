// geometry/Curve.cpp
#include "Curve.hpp"
#include <stdexcept>

namespace ndde::geometry {

std::vector<Sample> Curve::tessellate(std::uint32_t n) const {
    if (n == 0) throw std::invalid_argument("tessellate: n must be > 0");

    std::vector<Sample> samples;
    samples.reserve(n + 1);

    const float t0   = t_min();
    const float t1   = t_max();
    const float step = (t1 - t0) / static_cast<float>(n);

    for (std::uint32_t i = 0; i <= n; ++i) {
        const float t = t0 + static_cast<float>(i) * step;
        samples.push_back({t, evaluate(t)});
    }
    return samples;
}

std::vector<math::Vec4> Curve::vertex_buffer(std::uint32_t n) const {
    auto samples = tessellate(n);
    std::vector<math::Vec4> verts;
    verts.reserve(samples.size());
    for (const auto& s : samples) {
        verts.emplace_back(s.point.x, s.point.y, s.point.z, 1.f);
    }
    return verts;
}

} // namespace ndde::geometry
