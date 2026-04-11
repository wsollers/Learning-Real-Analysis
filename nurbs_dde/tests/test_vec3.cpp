// tests/test_vec3.cpp
#include "math/Vec3.hpp"
#include <gtest/gtest.h>
#include <cmath>

using namespace ndde::math;

TEST(Vec3, ZeroFactory) {
    auto v = zero3();
    EXPECT_FLOAT_EQ(v.x, 0.f);
    EXPECT_FLOAT_EQ(v.y, 0.f);
    EXPECT_FLOAT_EQ(v.z, 0.f);
}

TEST(Vec3, UnitVectors) {
    EXPECT_FLOAT_EQ(norm(unitX()), 1.f);
    EXPECT_FLOAT_EQ(norm(unitY()), 1.f);
    EXPECT_FLOAT_EQ(norm(unitZ()), 1.f);
}

TEST(Vec3, Norm) {
    Vec3 v{3.f, 4.f, 0.f};
    EXPECT_FLOAT_EQ(norm(v), 5.f);
}

TEST(Vec3, Dist) {
    Vec3 a{0.f, 0.f, 0.f};
    Vec3 b{1.f, 0.f, 0.f};
    EXPECT_FLOAT_EQ(dist(a, b), 1.f);
}

TEST(Vec3, Normalise) {
    Vec3 v{2.f, 0.f, 0.f};
    auto n = normalise(v);
    EXPECT_FLOAT_EQ(norm(n), 1.f);
    EXPECT_FLOAT_EQ(n.x, 1.f);
}

TEST(Vec3, ToString) {
    Vec3 v{1.f, 2.f, 3.f};
    auto s = to_string(v);
    EXPECT_FALSE(s.empty());
    // Should contain the values
    EXPECT_NE(s.find("1."), std::string::npos);
}
