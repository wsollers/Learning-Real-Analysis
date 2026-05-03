#include <gtest/gtest.h>

#include "app/ParticleBehaviors.hpp"
#include "app/ParticleSystem.hpp"
#include "math/Surfaces.hpp"

using ndde::ParticleRole;
using ndde::TrailMode;

namespace {

ndde::math::Paraboloid flat_surface() {
    return ndde::math::Paraboloid(0.001f, 10.f, -10.f, 10.f);
}

} // namespace

TEST(ParticleSystem, MetadataIncludesRoleAndBehaviors) {
    auto surface = flat_surface();
    ndde::ParticleSystem system(&surface, 1u);

    auto builder = system.factory().particle()
        .named("Probe")
        .role(ParticleRole::Avoider)
        .at({1.f, 0.f})
        .with_behavior<ndde::ConstantDriftBehavior>(glm::vec2{0.1f, 0.f})
        .with_behavior<ndde::BrownianBehavior>(ndde::BrownianBehavior::Params{
            .sigma = 0.05f,
            .drift_strength = 0.f
        });

    ndde::Particle& p = system.spawn(std::move(builder));
    const ndde::ParticleMetadata md = p.metadata();

    EXPECT_EQ(md.role, "Avoider");
    EXPECT_NE(p.metadata_label().find("Probe"), std::string::npos);
    EXPECT_NE(p.metadata_label().find("Constant Drift"), std::string::npos);
    EXPECT_NE(p.metadata_label().find("Brownian"), std::string::npos);
}

TEST(ParticleSystem, SeekBehaviorMovesTowardNearestRoleTarget) {
    auto surface = flat_surface();
    ndde::ParticleSystem system(&surface, 2u);

    system.spawn(system.factory().particle()
        .role(ParticleRole::Leader)
        .at({3.f, 0.f})
        .with_behavior<ndde::ConstantDriftBehavior>(glm::vec2{0.f, 0.f}));

    ndde::SeekParticleBehavior::Params seek;
    seek.target = ndde::TargetSelector::nearest(ParticleRole::Leader);
    seek.speed = 1.f;

    ndde::Particle& chaser = system.spawn(system.factory().particle()
        .role(ParticleRole::Chaser)
        .at({1.f, 0.f})
        .with_behavior<ndde::SeekParticleBehavior>(seek));

    const float before = chaser.head_uv().x;
    system.update(0.1f, 1.f, 0.1f);
    EXPECT_GT(chaser.head_uv().x, before);
}

TEST(ParticleSystem, AvoidBehaviorMovesAwayFromNearestRoleTarget) {
    auto surface = flat_surface();
    ndde::ParticleSystem system(&surface, 3u);

    system.spawn(system.factory().particle()
        .role(ParticleRole::Chaser)
        .at({3.f, 0.f})
        .with_behavior<ndde::ConstantDriftBehavior>(glm::vec2{0.f, 0.f}));

    ndde::AvoidParticleBehavior::Params avoid;
    avoid.target = ndde::TargetSelector::nearest(ParticleRole::Chaser);
    avoid.speed = 1.f;

    ndde::Particle& avoider = system.spawn(system.factory().particle()
        .role(ParticleRole::Avoider)
        .at({2.f, 0.f})
        .with_behavior<ndde::AvoidParticleBehavior>(avoid));

    const float before = avoider.head_uv().x;
    system.update(0.1f, 1.f, 0.1f);
    EXPECT_LT(avoider.head_uv().x, before);
}

TEST(ParticleSystem, CentroidSeekMovesTowardGroupCenter) {
    auto surface = flat_surface();
    ndde::ParticleSystem system(&surface, 4u);

    system.spawn(system.factory().particle()
        .role(ParticleRole::Chaser)
        .at({4.f, 0.f})
        .with_behavior<ndde::ConstantDriftBehavior>(glm::vec2{0.f, 0.f}));
    system.spawn(system.factory().particle()
        .role(ParticleRole::Chaser)
        .at({6.f, 0.f})
        .with_behavior<ndde::ConstantDriftBehavior>(glm::vec2{0.f, 0.f}));

    ndde::Particle& seeker = system.spawn(system.factory().particle()
        .role(ParticleRole::Leader)
        .at({1.f, 0.f})
        .with_behavior<ndde::CentroidSeekBehavior>(ndde::CentroidSeekBehavior::Params{
            .role = ParticleRole::Chaser,
            .speed = 1.f
        }));

    const float before = seeker.head_uv().x;
    system.update(0.1f, 1.f, 0.1f);
    EXPECT_GT(seeker.head_uv().x, before);
}

TEST(ParticleSystem, FiniteTrailTruncatesAndPersistentTrailDoesNot) {
    auto surface = flat_surface();
    ndde::ParticleSystem finite_system(&surface, 5u);
    ndde::Particle& finite = finite_system.spawn(finite_system.factory().particle()
        .role(ParticleRole::Neutral)
        .at({1.f, 0.f})
        .trail({TrailMode::Finite, 3u, 0.f})
        .with_behavior<ndde::ConstantDriftBehavior>(glm::vec2{0.2f, 0.f}));

    for (int i = 0; i < 12; ++i)
        finite_system.update(0.1f, 1.f, static_cast<float>(i + 1) * 0.1f);
    EXPECT_LE(finite.trail_vertex_count(), 3u);

    ndde::ParticleSystem persistent_system(&surface, 6u);
    ndde::Particle& persistent = persistent_system.spawn(persistent_system.factory().particle()
        .role(ParticleRole::Neutral)
        .at({1.f, 0.f})
        .trail({TrailMode::Persistent, 3u, 0.f})
        .with_behavior<ndde::ConstantDriftBehavior>(glm::vec2{0.2f, 0.f}));

    for (int i = 0; i < 12; ++i)
        persistent_system.update(0.1f, 1.f, static_cast<float>(i + 1) * 0.1f);
    EXPECT_GT(persistent.trail_vertex_count(), 3u);
}
