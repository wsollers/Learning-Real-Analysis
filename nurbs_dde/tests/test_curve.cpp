// tests/test_curve.cpp
// Tests the Curve base class behaviour via the Parabola concrete subclass.

#include "geometry/Parabola.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <stdexcept>

using namespace ndde::geometry;
using namespace ndde::math;

// ── Tessellation ───────────────────────────────────────────────────────────────

TEST(Curve, TessellateCountIsNPlusOne) {
    Parabola p;
    auto samples = p.tessellate(10);
    EXPECT_EQ(samples.size(), 11u);
}

TEST(Curve, TessellateFirstAndLastParameter) {
    Parabola p;
    auto samples = p.tessellate(100);
    EXPECT_FLOAT_EQ(samples.front().t, p.t_min());
    EXPECT_FLOAT_EQ(samples.back().t,  p.t_max());
}

TEST(Curve, TessellatePointsMatchEvaluate) {
    Parabola p;
    auto samples = p.tessellate(50);
    for (const auto& s : samples) {
        Vec3 expected = p.evaluate(s.t);
        EXPECT_NEAR(s.point.x, expected.x, 1e-6f);
        EXPECT_NEAR(s.point.y, expected.y, 1e-6f);
        EXPECT_NEAR(s.point.z, expected.z, 1e-6f);
    }
}

TEST(Curve, TessellateZeroThrows) {
    Parabola p;
    EXPECT_THROW((void)p.tessellate(0), std::invalid_argument);
}

TEST(Curve, TessellateParametersUniform) {
    Parabola p;
    constexpr std::uint32_t N = 10;
    auto samples = p.tessellate(N);
    const float expected_step = (p.t_max() - p.t_min()) / static_cast<float>(N);
    for (std::size_t i = 1; i < samples.size(); ++i) {
        float delta = samples[i].t - samples[i - 1].t;
        EXPECT_NEAR(delta, expected_step, 1e-5f);
    }
}

// ── Vertex buffer ──────────────────────────────────────────────────────────────

TEST(Curve, VertexBufferSize) {
    Parabola p;
    auto buf = p.vertex_buffer(64);
    EXPECT_EQ(buf.size(), 65u);  // n+1
}

TEST(Curve, VertexBufferWIsOne) {
    Parabola p;
    auto buf = p.vertex_buffer(10);
    for (const auto& v : buf) {
        EXPECT_FLOAT_EQ(v.w, 1.f);
    }
}

TEST(Curve, VertexBufferXYMatchCurve) {
    Parabola p;
    auto buf = p.vertex_buffer(10);
    auto samples = p.tessellate(10);
    ASSERT_EQ(buf.size(), samples.size());

    for (std::size_t i = 0; i < buf.size(); ++i) {
        EXPECT_NEAR(buf[i].x, samples[i].point.x, 1e-6f);
        EXPECT_NEAR(buf[i].y, samples[i].point.y, 1e-6f);
        EXPECT_NEAR(buf[i].z, samples[i].point.z, 1e-6f);
    }
}

// ── Domain ────────────────────────────────────────────────────────────────────

TEST(Curve, CustomDomain) {
    Parabola p{1.f, 0.f, 0.f, -3.f, 5.f};
    EXPECT_FLOAT_EQ(p.t_min(), -3.f);
    EXPECT_FLOAT_EQ(p.t_max(),  5.f);
    auto samples = p.tessellate(8);
    EXPECT_FLOAT_EQ(samples.front().t, -3.f);
    EXPECT_FLOAT_EQ(samples.back().t,   5.f);
}
