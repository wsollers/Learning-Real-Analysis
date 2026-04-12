//
// Created by wsoll on 4/11/2026.
//
#pragma once

#ifndef NURBS_DDE_TYPES_HPP
#define NURBS_DDE_TYPES_HPP


#include <array>
#include <cstdint>
#include <stdfloat>
#include <glm/glm.hpp>
#include <volk.h>

/**
 * @file Types.hpp
 * @brief Standardized fixed-width types for the NDDE engine.
 * * We use C++23 extended floating-point types to ensure binary
 * predictability between the CPU (C++) and GPU (SPIR-V/Vulkan).
 */

namespace ndde {

    // --- Floating Point Types (The Continuum) ---
    // Use f32 for rendering/storage, f64 for high-precision simulation/analysis.
    // --- Floating Point Types ---
    // MSVC support for std::float32_t is inconsistent.
    // We fall back to standard float/double which are IEEE-754 on x64.
#ifdef __STDCPP_FLOAT32_T__
    using f32 = std::float32_t;
#else
    using f32 = float;
#endif

#ifdef __STDCPP_FLOAT64_T__
    using f64 = std::float64_t;
#else
    using f64 = double;
#endif
    using f128 = long double;

    // --- Integer Types (The Discrete) ---
    using i8  = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using u8  = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    // --- Math Aliases (The Geometric Truth) ---
    // We leverage GLM for linear algebra because its memory layout
    // is natively compatible with Vulkan.
    using Vec2 = glm::vec2;
    using Vec3 = glm::vec3;
    using Vec4 = glm::vec4;
    using Mat4 = glm::mat4;


    struct Vertex {
        glm::vec3 pos;
        glm::vec4 color;

        // Helper to tell Vulkan how to read this struct
        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }

        // Add Attribute Descriptions here for Position and Color...
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
            // Position
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);
            // Color
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);
            return attributeDescriptions;
        }
    };

    struct PushConstants {
        Mat4 render_matrix; // The MVP matrix
        Vec4 color;         // The color to draw the curve
    };
}
#endif //NURBS_DDE_TYPES_HPP
