#include "app/SceneFactories.hpp"

#include "app/SimulationAnalysis.hpp"
#include "app/SimulationMultiWell.hpp"
#include "app/SimulationSurfaceGaussian.hpp"
#include "app/SimulationWavePredatorPrey.hpp"

namespace ndde {

void register_default_simulations(SimulationRegistry& registry) {
    registry.add(std::make_unique<SimulationRuntime>(
        "Surface Simulation",
        [] { return std::make_unique<SimulationSurfaceGaussian>(); }));
    registry.add(std::make_unique<SimulationRuntime>(
        "Sine-Rational Analysis",
        [] { return std::make_unique<SimulationAnalysis>(); }));
    registry.add(std::make_unique<SimulationRuntime>(
        "Multi-Well Centroid",
        [] { return std::make_unique<SimulationMultiWell>(); }));
    registry.add(std::make_unique<SimulationRuntime>(
        "Wave Predator-Prey",
        [] { return std::make_unique<SimulationWavePredatorPrey>(); }));
}

} // namespace ndde
