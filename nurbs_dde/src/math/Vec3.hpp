#pragma once
// math/Vec3.hpp
// Thin domain wrapper around glm::vec3.
// Long-term: R^3 coordinates for DDE trajectories, NURBS control points, etc.

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <format>
#include <string>

namespace ndde::math {

using Vec3 = glm::vec3;
using Vec2 = glm::vec2;
using Vec4 = glm::vec4;
using Mat4 = glm::mat4;

// ── Convenience factories ──────────────────────────────────────────────────────

inline constexpr Vec3 zero3()    { return {0.f, 0.f, 0.f}; }
inline constexpr Vec3 unitX()    { return {1.f, 0.f, 0.f}; }
inline constexpr Vec3 unitY()    { return {0.f, 1.f, 0.f}; }
inline constexpr Vec3 unitZ()    { return {0.f, 0.f, 1.f}; }

// ── R^3 metric ─────────────────────────────────────────────────────────────────

inline float norm(Vec3 v)        { return glm::length(v); }
inline float dist(Vec3 a, Vec3 b){ return glm::length(b - a); }
inline Vec3  normalise(Vec3 v)   { return glm::normalize(v); }

// ── Debug formatting ───────────────────────────────────────────────────────────

inline std::string to_string(Vec3 v) {
    return std::format("({:.4f}, {:.4f}, {:.4f})", v.x, v.y, v.z);
}

} // namespace ndde::math
