// tests/test_parabola.cpp
#include "geometry/Parabola.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <stdexcept>

using namespace ndde::geometry;
using namespace ndde::math;

// ── Construction ───────────────────────────────────────────────────────────────

TEST(Parabola, DefaultConstruction) {
    Parabola p;
    EXPECT_FLOAT_EQ(p.a(), 1.f);
    EXPECT_FLOAT_EQ(p.b(), 0.f);
    EXPECT_FLOAT_EQ(p.c(), 0.f);
    EXPECT_FLOAT_EQ(p.t_min(), -1.f);
    EXPECT_FLOAT_EQ(p.t_max(),  1.f);
}

TEST(Parabola, InvalidDomainThrows) {
    EXPECT_THROW(Parabola(1.f, 0.f, 0.f, 1.f, 0.f), std::invalid_argument);
    EXPECT_THROW(Parabola(1.f, 0.f, 0.f, 1.f, 1.f), std::invalid_argument);
}

// ── Evaluate ───────────────────────────────────────────────────────────────────

TEST(Parabola, VertexAtOrigin) {
    // y = x²: vertex at (0,0,0)
    Parabola p;
    Vec3 v = p.evaluate(0.f);
    EXPECT_FLOAT_EQ(v.x, 0.f);
    EXPECT_FLOAT_EQ(v.y, 0.f);
    EXPECT_FLOAT_EQ(v.z, 0.f);
}

TEST(Parabola, UnitParabolaSymmetry) {
    // y = x² is symmetric: p(-t) and p(t) have the same y, opposite x
    Parabola p;
    float t = 0.7f;
    Vec3 pos = p.evaluate( t);
    Vec3 neg = p.evaluate(-t);
    EXPECT_FLOAT_EQ(pos.y, neg.y);
    EXPECT_FLOAT_EQ(pos.x, -neg.x);
    EXPECT_FLOAT_EQ(pos.z,  neg.z);
}

TEST(Parabola, YEqualsXSquared) {
    Parabola p;
    for (float t : {-1.f, -0.5f, 0.f, 0.5f, 1.f}) {
        Vec3 v = p.evaluate(t);
        EXPECT_FLOAT_EQ(v.x, t);
        EXPECT_NEAR(v.y, t * t, 1e-6f);
        EXPECT_FLOAT_EQ(v.z, 0.f);
    }
}

TEST(Parabola, CustomCoefficients) {
    // y = 2t² + 3t + 1
    Parabola p{2.f, 3.f, 1.f};
    float t = 1.f;
    float expected_y = 2.f * t * t + 3.f * t + 1.f; // 6
    Vec3 v = p.evaluate(t);
    EXPECT_FLOAT_EQ(v.y, expected_y);
}

TEST(Parabola, XCoordEqualsParameter) {
    // The x coordinate is always equal to t
    Parabola p{3.f, -1.f, 2.f, -2.f, 2.f};
    for (float t : {-2.f, -1.f, 0.f, 1.f, 2.f}) {
        EXPECT_FLOAT_EQ(p.evaluate(t).x, t);
    }
}

TEST(Parabola, ZCoordAlwaysZero) {
    Parabola p;
    for (float t : {-1.f, 0.f, 1.f}) {
        EXPECT_FLOAT_EQ(p.evaluate(t).z, 0.f);
    }
}
