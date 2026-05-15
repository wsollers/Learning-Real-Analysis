#pragma once
// simulation/events/EngineEventTypes.hpp
// Canonical app-scoped event PODs fired on the EngineEventBus.
// engine/events/AppEvent.hpp is now a shim that includes this file.
// wall_time preserved for TelemetryService compatibility.

#include "math/Scalars.hpp"
#include <chrono>
#include <string_view>

namespace ndde::events {

struct AppStarted {
    std::string_view                      config_path;
    std::chrono::system_clock::time_point wall_time
        = std::chrono::system_clock::now();
};

struct AppStopping {};

struct SimSwitched {
    std::string_view from_name;
    std::string_view to_name;
    u64              sim_index = u64(0);
};

} // namespace ndde::events
