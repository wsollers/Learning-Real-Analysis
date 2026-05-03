#include "app/SimulationAnalysis.hpp"
#include "app/SimulationMultiWell.hpp"
#include "app/SimulationSurfaceGaussian.hpp"
#include "app/SimulationWavePredatorPrey.hpp"
#include "app/SceneFactories.hpp"
#include "engine/SimulationHost.hpp"

#include <gtest/gtest.h>
#include <glm/gtc/epsilon.hpp>

namespace {

using namespace ndde;

bool matrix_near_identity(const Mat4& matrix) {
    const Mat4 identity{1.f};
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            if (!glm::epsilonEqual(matrix[c][r], identity[c][r], 1e-5f))
                return false;
        }
    }
    return true;
}

template <class Sim>
void expect_simulation_registers_starts_and_emits_packets() {
    EngineServices services;
    SimulationHost host = services.simulation_host();
    Sim sim;

    sim.on_register(host);
    EXPECT_EQ(services.panels().active_count(), 4u);
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

    sim.on_stop();
    EXPECT_EQ(services.panels().active_count(), 0u);
    EXPECT_EQ(services.hotkeys().active_count(), 0u);
    EXPECT_EQ(services.render().active_view_count(), 0u);
}

TEST(AllSimulations, GaussianRegistersStartsAndEmitsPackets) {
    expect_simulation_registers_starts_and_emits_packets<SimulationSurfaceGaussian>();
}

TEST(AllSimulations, AnalysisRegistersStartsAndEmitsPackets) {
    expect_simulation_registers_starts_and_emits_packets<SimulationAnalysis>();
}

TEST(AllSimulations, MultiWellRegistersStartsAndEmitsPackets) {
    expect_simulation_registers_starts_and_emits_packets<SimulationMultiWell>();
}

TEST(AllSimulations, WavePredatorPreyRegistersStartsAndEmitsPackets) {
    expect_simulation_registers_starts_and_emits_packets<SimulationWavePredatorPrey>();
}

TEST(AllSimulations, DefaultRegistryContainsFourISimulationRuntimes) {
    SimulationRegistry registry;
    register_default_simulations(registry);

    ASSERT_EQ(registry.size(), 4u);
    EXPECT_EQ(registry.get(0)->name(), "Surface Simulation");
    EXPECT_EQ(registry.get(1)->name(), "Sine-Rational Analysis");
    EXPECT_EQ(registry.get(2)->name(), "Multi-Well Centroid");
    EXPECT_EQ(registry.get(3)->name(), "Wave Predator-Prey");
}

} // namespace
