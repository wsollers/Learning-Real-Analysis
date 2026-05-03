#include "engine/SimulationRuntime.hpp"

#include <stdexcept>
#include <utility>

namespace ndde {

SceneSimulationRuntime::SceneSimulationRuntime(std::string name, SceneFactory factory)
    : m_name(std::move(name))
    , m_factory(std::move(factory))
{
    publish();
}

void SceneSimulationRuntime::instantiate(EngineAPI api) {
    m_scene = m_factory(std::move(api));
    publish();
}

void SceneSimulationRuntime::start() {
    resume();
}

void SceneSimulationRuntime::pause() {
    if (m_scene) m_scene->set_paused(true);
    publish();
}

void SceneSimulationRuntime::resume() {
    if (!m_scene)
        throw std::runtime_error("[SimulationRuntime] start called before instantiate");
    m_scene->set_paused(false);
    publish();
}

bool SceneSimulationRuntime::paused() const noexcept {
    return m_scene ? m_scene->paused() : true;
}

SimulationSnapshot SceneSimulationRuntime::snapshot() const {
    return m_context.snapshot();
}

void SceneSimulationRuntime::publish() {
    m_context.publish(SimulationSnapshot{
        .name = m_name,
        .paused = paused()
    });
}

void SimulationRegistry::add(std::unique_ptr<SceneSimulationRuntime> runtime) {
    m_runtimes.push_back(std::move(runtime));
}

SceneSimulationRuntime* SimulationRegistry::get(std::size_t index) noexcept {
    if (index >= m_runtimes.size()) return nullptr;
    return m_runtimes[index].get();
}

const SceneSimulationRuntime* SimulationRegistry::get(std::size_t index) const noexcept {
    if (index >= m_runtimes.size()) return nullptr;
    return m_runtimes[index].get();
}

} // namespace ndde
