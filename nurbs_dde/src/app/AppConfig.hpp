//
// Created by wsoll on 4/11/2026.
//

#pragma once
#ifndef NURBS_DDE_APPCONFIG_HPP
#define NURBS_DDE_APPCONFIG_HPP

#include "core/Types.hpp"

namespace ndde {

    /**
     * @brief Global configuration and simulation state.
     * This struct holds the parameters that drive both the
     * mathematical analysis and the Vulkan rendering state.
     */
    struct AppConfig {
        // --- Rendering State ---
        bool is3D = false;             // Toggle between Orthographic (2D) and Perspective (3D)

        f32 nearPlane = 0.1f;
        f32 farPlane = 100.0f;

        // --- Camera State
        Vec3 cameraPosition = Vec3(0.0f, 0.0f, 5.0f); // Pan lives here
        Vec3 cameraTarget   = Vec3(0.0f, 0.0f, 0.0f);
        f32 zoomLevel       = 1.0f;                   // For Ortho (2D)
        f32 fov             = 45.0f;                  // For Perspective (3D)

        // --- Geometric Analysis ---
        // Your standardize_epsilon from notation.yaml
        f32 epsilon_snap = 0.05f;
        u32 tessellation_segments = 100;

        // --- Pursuit / DDE Parameters ---
        // The delay constant for the pursuit-delay chase
        f32 tau = 0.5f;
        f32 simulation_speed = 1.0f;
        bool is_paused = false;

        // --- Colors (Physicality) ---
        Vec4 curve_color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        Vec4 background_color = Vec4(0.1f, 0.1f, 0.1f, 1.0f);
    };

} // namespace ndde

#endif // NURBS_DDE_APPCONFIG_HPP