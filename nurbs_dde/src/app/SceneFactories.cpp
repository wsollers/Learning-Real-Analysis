// app/SceneFactories.cpp
// Registers the single active simulation.
// All other simulations have been archived to src/old/ and are not compiled.

#include "app/SceneFactories.hpp"
#include "app/SimulationWavePredatorPrey.hpp"

namespace ndde {

void register_default_simulations(SimulationRegistry& registry) {
    registry.add_runtime<SimulationWavePredatorPrey>("Wave Predator-Prey");
}

} // namespace ndde
