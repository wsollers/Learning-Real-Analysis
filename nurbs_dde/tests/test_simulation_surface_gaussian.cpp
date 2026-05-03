#include "app/SimulationSurfaceGaussian.hpp"
#include "engine/SimulationHost.hpp"

#include <gtest/gtest.h>

namespace {

using namespace ndde;

TEST(SimulationSurfaceGaussian, RegistersPanelsHotkeysAndViews) {
    EngineServices services;
    SimulationHost host = services.simulation_host();
    SimulationSurfaceGaussian sim;

    sim.on_register(host);

    EXPECT_EQ(services.panels().active_count(), 4u);
    EXPECT_EQ(services.hotkeys().active_count(), 2u);
    EXPECT_EQ(services.render().active_view_count(), 2u);
    EXPECT_TRUE(services.panels().contains("Sim - Controls"));
    EXPECT_TRUE(services.panels().contains("Sim - Swarms"));
    EXPECT_TRUE(services.panels().contains("Sim - Particles"));
    EXPECT_TRUE(services.panels().contains("Sim - Goals"));
    EXPECT_TRUE(services.hotkeys().contains("Reset Gaussian pursuit"));
    EXPECT_TRUE(services.render().contains_view("Surface 3D"));
    EXPECT_TRUE(services.render().contains_view("Surface Alternate"));

    sim.on_stop();
    EXPECT_EQ(services.panels().active_count(), 0u);
    EXPECT_EQ(services.hotkeys().active_count(), 0u);
    EXPECT_EQ(services.render().active_view_count(), 0u);
}

TEST(SimulationSurfaceGaussian, StartSpawnsParticlesAndTickEmitsRenderPackets) {
    EngineServices services;
    SimulationHost host = services.simulation_host();
    SimulationSurfaceGaussian sim;

    sim.on_register(host);
    sim.on_start();
    ASSERT_GT(sim.particle_count(), 0u);

    services.render().clear_packets();
    sim.on_tick(host.clock().next(1.f / 60.f));

    EXPECT_GT(services.render().packet_count(sim.main_view_id()), 0u);
    EXPECT_GT(services.render().packet_count(sim.alternate_view_id()), 0u);
    EXPECT_TRUE(sim.context().dirty().particles);
    EXPECT_TRUE(sim.context().dirty().main_view);
    EXPECT_TRUE(sim.context().dirty().alternate_view);

    const SceneSnapshot snapshot = sim.snapshot();
    EXPECT_EQ(snapshot.name, "Surface Simulation");
    EXPECT_EQ(snapshot.particle_count, sim.particle_count());
    EXPECT_GT(snapshot.sim_time, 0.f);
}

TEST(SimulationSurfaceGaussian, HotkeysInvokeSimulationCommands) {
    EngineServices services;
    SimulationHost host = services.simulation_host();
    SimulationSurfaceGaussian sim;

    sim.on_register(host);
    sim.on_start();
    const std::size_t initial_count = sim.particle_count();

    EXPECT_TRUE(services.hotkeys().dispatch({.key = 'B', .mods = 2}));
    EXPECT_GT(sim.particle_count(), initial_count);

    EXPECT_TRUE(services.hotkeys().dispatch({.key = 'R', .mods = 2}));
    EXPECT_GT(sim.particle_count(), 0u);
}

TEST(SimulationSurfaceGaussian, PerturbationCommandSurvivesUntilTickAndMarksDirty) {
    EngineServices services;
    SimulationHost host = services.simulation_host();
    SimulationSurfaceGaussian sim;

    sim.on_register(host);
    sim.on_start();
    sim.context().clear_frame_state();
    sim.context().queue_perturbation(SurfacePerturbation{
        .uv = {0.25f, -0.25f},
        .amplitude = 0.1f,
        .radius = 0.8f,
        .falloff = 1.f,
        .seed = 9u
    });

    ASSERT_TRUE(sim.context().has_pending_perturbations());
    EXPECT_TRUE(sim.context().dirty().surface);
    sim.on_tick(host.clock().next(1.f / 60.f));
    EXPECT_FALSE(sim.context().has_pending_perturbations());
    EXPECT_TRUE(sim.context().dirty().surface);
}

TEST(SimulationSurfaceGaussian, RenderSurfacePerturbationCommandMarksSurfaceDirty) {
    EngineServices services;
    SimulationHost host = services.simulation_host();
    SimulationSurfaceGaussian sim;

    sim.on_register(host);
    sim.on_start();
    sim.context().clear_frame_state();

    services.render().queue_surface_perturbation(SurfacePerturbCommand{
        .view = sim.main_view_id(),
        .uv = {0.4f, -0.6f},
        .amplitude = 0.2f,
        .radius = 0.75f,
        .falloff = 1.2f,
        .seed = 12u
    });

    sim.on_tick(host.clock().next(1.f / 60.f));
    EXPECT_TRUE(sim.context().dirty().surface);
    EXPECT_TRUE(sim.context().math_cache().surface_revision > 0u);
    EXPECT_FALSE(sim.context().has_pending_perturbations());
}

} // namespace
