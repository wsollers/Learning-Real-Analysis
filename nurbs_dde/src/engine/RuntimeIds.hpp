#pragma once
// engine/RuntimeIds.hpp
// Shared stable identifiers used by metadata, diagnostics, scenarios, and telemetry.

#include "math/Scalars.hpp"

#include <string_view>

namespace ndde {

struct ComponentId {
    std::string_view value{};

    friend constexpr bool operator==(ComponentId, ComponentId) noexcept = default;
};

struct RuntimeNodeId {
    u64 value = u64(0);

    friend constexpr bool operator==(RuntimeNodeId, RuntimeNodeId) noexcept = default;
};

struct StreamId {
    std::string_view value{};

    friend constexpr bool operator==(StreamId, StreamId) noexcept = default;
};

struct EventTypeId {
    std::string_view value{};

    friend constexpr bool operator==(EventTypeId, EventTypeId) noexcept = default;
};

struct ResourceId {
    u64 value = u64(0);

    friend constexpr bool operator==(ResourceId, ResourceId) noexcept = default;
};

struct CaptureArtifactId {
    u64 value = u64(0);

    friend constexpr bool operator==(CaptureArtifactId, CaptureArtifactId) noexcept = default;
};

namespace ids {

inline constexpr ComponentId unknown_component{"unknown"};
inline constexpr ComponentId simulation_wave_predator_prey{"simulation.wave_predator_prey"};
inline constexpr ComponentId surface_torus{"surface.torus"};
inline constexpr ComponentId surface_sine_rational{"surface.sine_rational"};
inline constexpr ComponentId surface_wave_predator_prey{"surface.wave_predator_prey"};
inline constexpr ComponentId field_metric_ripple{"field.metric_ripple"};
inline constexpr ComponentId field_damping{"field.damping"};
inline constexpr ComponentId behavior_brownian{"behavior.brownian"};
inline constexpr ComponentId behavior_seek{"behavior.seek"};
inline constexpr ComponentId behavior_avoid{"behavior.avoid"};
inline constexpr ComponentId solver_ode_euler{"solver.ode.euler"};
inline constexpr ComponentId solver_ode_rk4{"solver.ode.rk4"};
inline constexpr ComponentId system_gravity_pendulum{"system.gravity.pendulum"};
inline constexpr ComponentId system_gravity_planar_n_body{"system.gravity.planar_n_body"};

} // namespace ids

} // namespace ndde
