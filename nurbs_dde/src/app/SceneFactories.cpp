#include "app/SceneFactories.hpp"

#include "app/AnalysisScene.hpp"
#include "app/SurfaceSimScene.hpp"

#include <utility>

namespace ndde {

std::unique_ptr<IScene> make_surface_sim_scene(EngineAPI api) {
    return std::make_unique<SurfaceSimScene>(std::move(api));
}

std::unique_ptr<IScene> make_analysis_scene(EngineAPI api) {
    return std::make_unique<AnalysisScene>(std::move(api));
}

} // namespace ndde
