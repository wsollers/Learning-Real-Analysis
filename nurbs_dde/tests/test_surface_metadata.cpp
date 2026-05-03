#include "app/GaussianRipple.hpp"
#include "app/SurfaceRegistry.hpp"
#include "math/SineRationalSurface.hpp"
#include "math/Surfaces.hpp"

#include <gtest/gtest.h>

TEST(SurfaceMetadata, SineRationalExposesFormulaDomainAndDerivativeSupport) {
    ndde::math::SineRationalSurface surface{4.f};
    const ndde::math::SurfaceMetadata metadata = surface.metadata();

    EXPECT_EQ(metadata.name, "Sine-Rational Surface");
    EXPECT_FALSE(metadata.formula.empty());
    EXPECT_TRUE(metadata.has_analytic_derivatives);
    EXPECT_FALSE(metadata.deformable);
    EXPECT_FLOAT_EQ(metadata.domain.u_min, -4.f);
    EXPECT_FLOAT_EQ(metadata.domain.u_max, 4.f);
    ASSERT_EQ(metadata.parameter_count, 1u);
    EXPECT_EQ(metadata.parameters[0].name, "extent");
}

TEST(SurfaceMetadata, GaussianRippleReportsDeformableParameters) {
    ndde::GaussianRipple ripple;
    const ndde::math::SurfaceMetadata metadata = ripple.metadata();

    EXPECT_EQ(metadata.name, "Gaussian Ripple");
    EXPECT_TRUE(metadata.deformable);
    EXPECT_TRUE(metadata.time_varying);
    ASSERT_GE(metadata.parameter_count, 5u);
    EXPECT_EQ(metadata.parameters[0].name, "amplitude");
}

TEST(SurfaceMetadata, AppRegistrySurfacesExposeNames) {
    const auto multi = ndde::SurfaceRegistry::make_multi_well();
    const auto wave = ndde::SurfaceRegistry::make_wave_predator_prey();

    EXPECT_EQ(multi->metadata().name, "Multi-Well Wave Surface");
    EXPECT_EQ(wave->metadata().name, "Wave Predator-Prey Surface");
}
