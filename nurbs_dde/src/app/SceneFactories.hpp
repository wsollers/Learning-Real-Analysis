#pragma once
// app/SceneFactories.hpp
// Type-erased scene construction helpers used by Engine and scene switch UI.

#include "engine/EngineAPI.hpp"
#include "engine/IScene.hpp"
#include "engine/SimulationRuntime.hpp"
#include <memory>

namespace ndde {

[[nodiscard]] std::unique_ptr<IScene> make_surface_sim_scene(EngineAPI api);
[[nodiscard]] std::unique_ptr<IScene> make_analysis_scene(EngineAPI api);
[[nodiscard]] std::unique_ptr<IScene> make_multiwell_scene(EngineAPI api);
[[nodiscard]] std::unique_ptr<IScene> make_wave_predator_prey_scene(EngineAPI api);

void register_default_simulations(SimulationRegistry& registry);

} // namespace ndde
