//
// Created by wsoll on 4/11/2026.
//
#pragma once

#ifndef NURBS_DDE_ENGINECONTEXT_HPP
#define NURBS_DDE_ENGINECONTEXT_HPP


#include "core/Types.hpp"
#include <memory>

namespace ndde {

    // Forward declarations to keep the header "thin"
    class VulkanRenderer;
    class BufferManager;
    struct AppConfig;

    /**
     * @brief The EngineContext acts as a Service Locator.
     * It owns the lifecycle of core engine systems and provides
     * access to them across the application.
     */
    struct EngineContext {
        // Source of Truth for simulation settings (2D/3D, tau, epsilon)
        std::unique_ptr<AppConfig> config;

        // Low-level Vulkan management
        std::unique_ptr<VulkanRenderer> renderer;

        // Centralized memory allocation for vertices
        std::unique_ptr<BufferManager> buffers;

        // Constructor handles the dependency injection
        EngineContext();
        ~EngineContext();

        // Prevent copying to avoid multiple "owners" of the GPU
        EngineContext(const EngineContext&) = delete;
        EngineContext& operator=(const EngineContext&) = delete;
    };

} // namespace ndde
#endif //NURBS_DDE_ENGINECONTEXT_HPP
