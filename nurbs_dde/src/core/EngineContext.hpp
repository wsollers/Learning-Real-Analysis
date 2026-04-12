#pragma once
#include <memory>
#include <volk.h>
#include "app/AppConfig.hpp"
#include "core/BufferManager.hpp"
#include "renderer/VulkanRenderer.hpp" // MUST be included for unique_ptr

namespace ndde {

    class EngineContext {
    public:
        EngineContext(const AppConfig& config);
        ~EngineContext();

        // Application interface
        bool is_running() const;
        void new_frame();
        void end_frame();
        void submit_frame();
        void draw_geometry(const ArenaSlice& slice);
        // Getters for Application
        float get_delta_time() const { return 0.016f; } 
        ArenaSlice acquire_geometry_slice(uint32_t count) { 
            return m_buffer_manager->acquire_large_slice(count); 
        }

    private:
        AppConfig m_config;
        std::unique_ptr<VulkanRenderer> m_renderer;
        std::unique_ptr<BufferManager> m_buffer_manager;
    };

} // namespace ndde