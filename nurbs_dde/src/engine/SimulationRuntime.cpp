#include "engine/SimulationRuntime.hpp"

#include <stdexcept>
#include <utility>

namespace ndde {

SimulationRuntime::SimulationRuntime(std::string name, SimulationFactory factory)
    : m_name(std::move(name))
    , m_factory(std::move(factory))
{
    publish();
}

SimulationRuntime::~SimulationRuntime() {
    stop();
}

void SimulationRuntime::instantiate(SimulationHost& host) {
    stop();
    m_simulation = m_factory();
    m_simulation->on_register(host);
    m_started = false;
    m_paused = true;
    publish();
}

void SimulationRuntime::start() {
    if (!m_simulation)
        throw std::runtime_error("[SimulationRuntime] start called before instantiate");
    if (!m_started) {
        m_simulation->on_start();
        m_started = true;
    }
    resume();
}

void SimulationRuntime::stop() {
    if (m_simulation) {
        m_simulation->on_stop();
        m_simulation.reset();
    }
    m_started = false;
    m_paused = true;
    publish();
}

void SimulationRuntime::pause() {
    m_paused = true;
    publish();
}

void SimulationRuntime::resume() {
    if (!m_simulation)
        throw std::runtime_error("[SimulationRuntime] resume called before instantiate");
    m_paused = false;
    publish();
}

void SimulationRuntime::tick(TickInfo tick) {
    if (!m_simulation) return;
    TickInfo effective = tick;
    effective.paused = m_paused || tick.paused;
    if (effective.paused) effective.dt = 0.f;
    m_simulation->on_tick(effective);
    publish();
}

void SimulationRuntime::publish() {
    if (m_simulation) {
        SimulationSnapshot snapshot = m_simulation->snapshot();
        snapshot.paused = m_paused;
        m_snapshots.publish(std::move(snapshot));
        return;
    }
    m_snapshots.publish(SimulationSnapshot{
        .name = m_name,
        .paused = true,
        .status = "Not instantiated"
    });
}

ISimulation& SimulationRuntime::simulation() {
    if (!m_simulation)
        throw std::runtime_error("[SimulationRuntime] simulation requested before instantiate");
    return *m_simulation;
}

const ISimulation& SimulationRuntime::simulation() const {
    if (!m_simulation)
        throw std::runtime_error("[SimulationRuntime] simulation requested before instantiate");
    return *m_simulation;
}

SimulationSnapshot SimulationRuntime::snapshot() const {
    return m_snapshots.snapshot();
}

SimulationMetadata SimulationRuntime::metadata() const {
    if (!m_simulation) {
        return SimulationMetadata{
            .name = m_name,
            .status = "Not instantiated",
            .paused = true
        };
    }
    SimulationMetadata data = m_simulation->metadata();
    data.paused = m_paused;
    return data;
}

void SimulationRegistry::add(std::unique_ptr<SimulationRuntime> runtime) {
    m_runtimes.push_back(std::move(runtime));
}

SimulationRuntime* SimulationRegistry::get(std::size_t index) noexcept {
    if (index >= m_runtimes.size()) return nullptr;
    return m_runtimes[index].get();
}

const SimulationRuntime* SimulationRegistry::get(std::size_t index) const noexcept {
    if (index >= m_runtimes.size()) return nullptr;
    return m_runtimes[index].get();
}

} // namespace ndde
