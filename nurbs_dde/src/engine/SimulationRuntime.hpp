#pragma once
// engine/SimulationRuntime.hpp
// First-class switchable simulation runtime.

#include "engine/EngineAPI.hpp"
#include "engine/IScene.hpp"
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

class ISimulationRuntime {
public:
    virtual ~ISimulationRuntime() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;
    [[nodiscard]] virtual IScene& scene() = 0;
    [[nodiscard]] virtual const IScene& scene() const = 0;

    virtual void start() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void publish() = 0;
    [[nodiscard]] virtual bool paused() const noexcept = 0;
    [[nodiscard]] virtual SimulationSnapshot snapshot() const = 0;
};

class SceneSimulationRuntime final : public ISimulationRuntime {
public:
    using SceneFactory = std::function<std::unique_ptr<IScene>(EngineAPI)>;

    SceneSimulationRuntime(std::string name, SceneFactory factory);

    void instantiate(EngineAPI api);

    [[nodiscard]] std::string_view name() const override { return m_name; }
    [[nodiscard]] IScene& scene() override { return *m_scene; }
    [[nodiscard]] const IScene& scene() const override { return *m_scene; }

    void start() override;
    void pause() override;
    void resume() override;
    void publish() override;
    [[nodiscard]] bool paused() const noexcept override;
    [[nodiscard]] SimulationSnapshot snapshot() const override;

private:
    std::string m_name;
    SceneFactory m_factory;
    std::unique_ptr<IScene> m_scene;
    SimulationSnapshotStore m_snapshots;

};

class SimulationRegistry {
public:
    void add(std::unique_ptr<SceneSimulationRuntime> runtime);

    [[nodiscard]] std::size_t size() const noexcept { return m_runtimes.size(); }
    [[nodiscard]] SceneSimulationRuntime* get(std::size_t index) noexcept;
    [[nodiscard]] const SceneSimulationRuntime* get(std::size_t index) const noexcept;

private:
    std::vector<std::unique_ptr<SceneSimulationRuntime>> m_runtimes;
};

} // namespace ndde
