#pragma once
// engine/ISimulation.hpp
// First-class simulation contract for the next architecture.

#include "engine/IScene.hpp"
#include "engine/SimulationMetadata.hpp"
#include "engine/SimulationClock.hpp"
#include "engine/SimulationHost.hpp"

#include <string_view>

namespace ndde {

class ISimulation {
public:
    virtual ~ISimulation() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;

    virtual void on_register(SimulationHost& host) = 0;
    virtual void on_start() = 0;
    virtual void on_tick(const TickInfo& tick) = 0;
    virtual void on_stop() = 0;

    [[nodiscard]] virtual SceneSnapshot snapshot() const {
        return SceneSnapshot{ .name = std::string(name()) };
    }

    [[nodiscard]] virtual SimulationMetadata metadata() const {
        const SceneSnapshot s = snapshot();
        return SimulationMetadata{
            .name = s.name,
            .status = s.status,
            .sim_time = s.sim_time,
            .sim_speed = s.sim_speed,
            .particle_count = s.particle_count,
            .paused = s.paused,
            .goal_succeeded = s.status == "Succeeded"
        };
    }
};

} // namespace ndde
