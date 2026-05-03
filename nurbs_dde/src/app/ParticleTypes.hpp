#pragma once
// app/ParticleTypes.hpp
// Shared particle vocabulary: roles, metadata, and visual trail policy.

#include "math/Scalars.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ndde {

using ParticleId = std::uint64_t;

enum class ParticleRole : u8 {
    Neutral,
    Leader,
    Chaser,
    Avoider
};

[[nodiscard]] inline std::string_view role_name(ParticleRole role) noexcept {
    switch (role) {
        case ParticleRole::Leader: return "Leader";
        case ParticleRole::Chaser: return "Chaser";
        case ParticleRole::Avoider: return "Avoider";
        case ParticleRole::Neutral: default: return "Neutral";
    }
}

enum class TrailMode : u8 {
    None,
    Finite,
    Persistent,
    StaticCurve
};

struct TrailConfig {
    TrailMode mode = TrailMode::Finite;
    u32       max_points = 1200;
    float     min_spacing = 0.015f;
};

struct ParticleMetadata {
    std::string              label;
    std::string              role;
    std::vector<std::string> behaviors;
    std::vector<std::string> constraints;
    std::vector<std::string> goals;
};

} // namespace ndde
