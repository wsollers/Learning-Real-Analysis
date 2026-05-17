// tests/test_all_simulations.cpp
// Tests for the active simulation set.
// Old simulations (Analysis, MultiWell, Gaussian, Differential*) have been
// archived to src/old/ -- only SimulationWavePredatorPrey is active.

#include "app/Curve2DOverlay.hpp"
#include "app/SimulationWavePredatorPrey.hpp"
#include "app/SceneFactories.hpp"
#include "engine/SimulationHost.hpp"

#include <gtest/gtest.h>
#include <glm/gtc/epsilon.hpp>

namespace {

using namespace ndde;

bool matrix_near_identity(const Mat4& matrix) {
    const Mat4 identity{1.f};
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            if (!glm::epsilonEqual(matrix[c][r], identity[c][r], 1e-5f))
                return false;
    return true;
}

template <class Sim>
void expect_simulation_registers_starts_and_emits_packets() {
    EngineServices services;
    SimulationHost host = services.simulation_host();
    Sim sim;

    sim.on_register(host);
    EXPECT_GE(services.panels().active_count(), 2u);
    EXPECT_GE(services.hotkeys().active_count(), 2u);
    EXPECT_EQ(services.render().active_view_count(), 2u);

    sim.on_start();
    EXPECT_GT(sim.particle_count(), 0u);

    services.render().clear_packets();
    sim.on_tick(host.clock().next(1.f / 60.f));

    EXPECT_GT(services.render().packet_count(sim.main_view_id()), 0u);
    EXPECT_GT(services.render().packet_count(sim.alternate_view_id()), 0u);
    ASSERT_FALSE(services.render().packets().empty());
    EXPECT_FALSE(matrix_near_identity(services.render().packets().front().mvp));
    EXPECT_EQ(sim.snapshot().particle_count, sim.particle_count());
    const SimulationMetadata metadata = sim.metadata();
    EXPECT_EQ(metadata.name, sim.name());
    EXPECT_FALSE(metadata.surface_name.empty());
    EXPECT_EQ(metadata.particle_count, sim.particle_count());
    EXPECT_FALSE(services.events().log(EventChannelId::Simulation).entries().empty());

    sim.on_stop();
    EXPECT_EQ(services.panels().active_count(), 0u);
    EXPECT_EQ(services.hotkeys().active_count(), 0u);
    EXPECT_EQ(services.render().active_view_count(), 0u);
}

TEST(AllSimulations, WavePredatorPreyRegistersStartsAndEmitsPackets) {
    expect_simulation_registers_starts_and_emits_packets<SimulationWavePredatorPrey>();
}

TEST(AllSimulations, DefaultRegistryContainsOnlyWavePredatorPrey) {
    EngineServices services;
    SimulationRegistry registry(services.memory());
    register_default_simulations(registry);

    ASSERT_EQ(registry.size(), 1u);
    EXPECT_EQ(registry.get(0)->name(), "Wave Predator-Prey");
}

TEST(AllSimulations, WavePredatorPreyDoubleClickSurfacePickAddsRipple) {
    EngineServices services;
    SimulationHost host = services.simulation_host();
    SimulationWavePredatorPrey sim;

    sim.on_register(host);
    sim.on_start();
    const std::size_t before = sim.active_field_count();

    ASSERT_TRUE(host.camera().queue_surface_perturbation(
        host.interaction(),
        sim.main_view_id(),
        Vec2{0.5f, 0.5f},
        Vec2{0.f, 0.f},
        99u));

    sim.on_tick(TickInfo{
        .tick_index = 1u,
        .dt = 0.f,
        .time = 0.f,
        .paused = true
    });

    host.threads().drain_service_mailboxes();
    host.events().drain(EventChannelId::Simulation, f32(0.1f), u64(2));

    EXPECT_EQ(sim.active_field_count(), before + 1u);
    EXPECT_TRUE(host.interaction().consume_surface_picks(sim.main_view_id()).empty());
    EXPECT_FALSE(host.events().log(EventChannelId::Simulation).entries().empty());

    sim.on_stop();
}

TEST(AllSimulations, WavePredatorPreySurfacePokeCommandAddsRippleWithoutInteractionService) {
    EngineServices services;
    SimulationHost host = services.simulation_host();
    SimulationWavePredatorPrey sim;

    sim.on_register(host);
    sim.on_start();
    const std::size_t before = sim.active_field_count();

    sim.on_simulation_command(SimulationThreadCommand{
        .kind = SimulationThreadCommandKind::SurfacePoke,
        .tick = TickInfo{.tick_index = 2u, .time = f32(0.1f), .paused = false},
        .surface_poke = SimulationSurfacePoke{
            .view = sim.main_view_id(),
            .uv = Vec2{f32(0.25), f32(-0.5)},
            .fallback_uv = Vec2{f32(0.25), f32(-0.5)},
            .amplitude = f32(0.25),
            .radius = f32(1),
            .falloff = f32(1),
            .seed = u32(101)
        }
    });

    EXPECT_EQ(sim.active_field_count(), before + 1u);
    EXPECT_TRUE(host.interaction().consume_surface_picks(sim.main_view_id()).empty());

    host.threads().drain_service_mailboxes();
    host.events().drain(EventChannelId::Simulation, f32(0.1f), u64(2));

    const auto& entries = host.events().log(EventChannelId::Simulation).entries();
    ASSERT_GE(entries.size(), 2u);
    EXPECT_EQ(entries[entries.size() - 2u].kind, events::EventKind::PerturbationFired);
    EXPECT_EQ(entries[entries.size() - 1u].kind, events::EventKind::FieldAdded);

    sim.on_stop();
}

TEST(AllSimulations, Curve2DHoverOverlayBuildsFrenetAndOsculatingGeometryNearCurve) {
    EngineServices services;
    const Vec2 curve[] = {
        {-1.f, 1.f}, {-0.5f, 0.25f}, {0.f, 0.f}, {0.5f, 0.25f}, {1.f, 1.f}
    };
    auto overlay = build_curve2d_frenet_hover_overlay(
        std::span<const Vec2>{curve, 5u},
        {0.f, 0.f},
        RenderViewDomain{.u_min = -2.f, .u_max = 2.f, .v_min = -1.f, .v_max = 2.f},
        true, true,
        &services.memory());
    EXPECT_GT(overlay.size(), 4u);
}

TEST(AllSimulations, Curve2DHoverOverlayBuildsVelocityAndDelayDiagnostics) {
    EngineServices services;
    const Vec2 curve[] = {
        {0.f, 0.f}, {0.5f, 0.25f}, {1.f, 0.5f}, {1.5f, 0.25f}, {2.f, 0.f}
    };
    const Curve2DHoverOverlayOptions options{
        .show_frenet = true,
        .show_velocity_arrow = true,
        .show_delay_ghost = true,
        .has_velocity = true,
        .has_delay_ghost = true,
        .velocity = {1.f, 0.4f},
        .delay_ghost = {0.5f, 0.25f}
    };
    auto overlay = build_curve2d_hover_overlay(
        std::span<const Vec2>{curve, 5u},
        {1.f, 0.5f},
        RenderViewDomain{.u_min = 0.f, .u_max = 2.f, .v_min = -1.f, .v_max = 1.f},
        options,
        &services.memory());
    EXPECT_TRUE(overlay.snapped);
    EXPECT_EQ(overlay.sample_index, 2u);
    EXPECT_GT(overlay.vertices.size(), 20u);
}

} // namespace
