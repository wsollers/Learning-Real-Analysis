#pragma once
// math/Types.hpp
// Canonical fixed-width scalar and vector types for the NDDE engine.
//
// Design contract:
//   - f32 / f64 are IEEE-754 on every target we care about (x64 Windows/Linux).
//   - Vec2/3/4 and Mat4 have GLM memory layout, which is natively compatible
//     with SPIR-V/Vulkan without any reinterpret_cast gymnastics.
//   - Vertex carries both a position and a per-vertex colour so the GPU can
//     use a single vertex buffer for heterogeneous draw calls (axes, curves,
//     surfaces, stochastic paths).
//   - PushConstants must stay <= 128 bytes (guaranteed minimum by the Vulkan
//     spec). Current layout: 64 (mat4) + 16 (vec4) = 80 bytes. Safe.

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <volk.h>

namespace ndde {

// ── Scalars ───────────────────────────────────────────────────────────────────

using f32  = float;          // IEEE-754 single — GPU native
using f64  = double;         // IEEE-754 double — simulation / analysis
using f128 = long double;    // Extended — stochastic integration only

using i8   = std::int8_t;
using i16  = std::int16_t;
using i32  = std::int32_t;
using i64  = std::int64_t;

using u8   = std::uint8_t;
using u16  = std::uint16_t;
using u32  = std::uint32_t;
using u64  = std::uint64_t;

// ── Linear algebra (GLM aliased) ──────────────────────────────────────────────

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat4 = glm::mat4;

// ── GPU vertex layout ─────────────────────────────────────────────────────────
// Binding 0, stride = sizeof(Vertex) = 28 bytes
// location 0: pos   (vec3, R32G32B32_SFLOAT, offset 0)
// location 1: color (vec4, R32G32B32A32_SFLOAT, offset 12)

struct Vertex {
    Vec3 pos;
    Vec4 color;

    [[nodiscard]] static VkVertexInputBindingDescription binding_description() noexcept {
        return {
            .binding   = 0,
            .stride    = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
    }

    [[nodiscard]] static std::array<VkVertexInputAttributeDescription, 2>
    attribute_descriptions() noexcept {
        return {{
            { .location = 0, .binding = 0,
              .format = VK_FORMAT_R32G32B32_SFLOAT,
              .offset = offsetof(Vertex, pos)   },
            { .location = 1, .binding = 0,
              .format = VK_FORMAT_R32G32B32A32_SFLOAT,
              .offset = offsetof(Vertex, color) }
        }};
    }
};

// ── Push constants (<= 128 bytes) ─────────────────────────────────────────────
// Sent once per draw call via vkCmdPushConstants.
// mode == 0: use per-vertex color from the vertex buffer.
// mode == 1: override with uniform_color for all fragments.

struct PushConstants {
    Mat4 mvp;            // 64 bytes
    Vec4 uniform_color;  // 16 bytes  -> total 80 bytes
    i32  mode;           // 4 bytes   -> total 84 bytes
    // 44 bytes headroom remaining before the 128-byte spec limit
};
static_assert(sizeof(PushConstants) <= 128,
    "PushConstants exceeds Vulkan guaranteed minimum push constant size");

// ── Draw mode ─────────────────────────────────────────────────────────────────

enum class DrawMode : i32 {
    VertexColor  = 0,   ///< per-vertex colour attribute
    UniformColor = 1,   ///< PushConstants::uniform_color for every fragment
};

// ── Topology hint ─────────────────────────────────────────────────────────────
// Passed alongside geometry slices so the renderer can select the right
// pipeline without the math layer knowing about VkPrimitiveTopology.

enum class Topology {
    LineList,       ///< axes, grids — pairs of vertices form segments
    LineStrip,      ///< curves — contiguous strip
    TriangleList,   ///< surfaces — future use
};

} // namespace ndde
