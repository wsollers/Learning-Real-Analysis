#include "app/SceneFactories.hpp"

#include "app/SimulationAnalysis.hpp"
#include "app/SimulationMultiWell.hpp"
#include "app/SimulationSurfaceGaussian.hpp"
#include "app/SimulationWavePredatorPrey.hpp"

namespace ndde {

void register_default_simulations(SimulationRegistry& registry) {
    registry.add_runtime<SimulationSurfaceGaussian>("Surface Simulation");
    registry.add_runtime<SimulationAnalysis>("Sine-Rational Analysis");
    registry.add_runtime<SimulationMultiWell>("Multi-Well Centroid");
    registry.add_runtime<SimulationWavePredatorPrey>("Wave Predator-Prey");
}

} // namespace ndde
