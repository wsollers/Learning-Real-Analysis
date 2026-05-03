#pragma once
// engine/SimulationRuntime.hpp
// ISimulation-only runtime.  The old IScene adapter path is intentionally gone.

#include "engine/ISimulation.hpp"
#include "engine/SimulationHost.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace ndde {

using SimulationSnapshot = SceneSnapshot;

class SimulationSnapshotStore {
public:
    void publish(SimulationSnapshot snapshot) {
        std::scoped_lock lock(m_mutex);
        m_snapshot = std::move(snapshot);
    }

    [[nodiscard]] SimulationSnapshot snapshot() const {
        std::scoped_lock lock(m_mutex);
        return m_snapshot;
    }

private:
    mutable std::mutex m_mutex;
    SimulationSnapshot m_snapshot;
};

class SimulationRuntime final {
public:
    using SimulationFactory = std::function<std::unique_ptr<ISimulation>()>;

    SimulationRuntime(std::string name, SimulationFactory factory);
    ~SimulationRuntime();

    SimulationRuntime(const SimulationRuntime&) = delete;
    SimulationRuntime& operator=(const SimulationRuntime&) = delete;

    void instantiate(SimulationHost& host);
    void start();
    void stop();
    void pause();
    void resume();
    void tick(TickInfo tick);
    void publish();

    [[nodiscard]] std::string_view name() const noexcept { return m_name; }
    [[nodiscard]] bool paused() const noexcept { return m_paused; }
    [[nodiscard]] bool instantiated() const noexcept { return static_cast<bool>(m_simulation); }
    [[nodiscard]] ISimulation& simulation();
    [[nodiscard]] const ISimulation& simulation() const;
    [[nodiscard]] SimulationSnapshot snapshot() const;
    [[nodiscard]] SimulationMetadata metadata() const;

private:
    std::string m_name;
    SimulationFactory m_factory;
    std::unique_ptr<ISimulation> m_simulation;
    SimulationSnapshotStore m_snapshots;
    bool m_started = false;
    bool m_paused = true;
};

class SimulationRegistry {
public:
    void add(std::unique_ptr<SimulationRuntime> runtime);

    [[nodiscard]] std::size_t size() const noexcept { return m_runtimes.size(); }
    [[nodiscard]] SimulationRuntime* get(std::size_t index) noexcept;
    [[nodiscard]] const SimulationRuntime* get(std::size_t index) const noexcept;

private:
    std::vector<std::unique_ptr<SimulationRuntime>> m_runtimes;
};

} // namespace ndde
