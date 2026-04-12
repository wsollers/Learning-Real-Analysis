#include "core/EngineContext.hpp"

namespace ndde {

    EngineContext::EngineContext(const AppConfig& config)
        : m_config(config)
    {
        // 1. Initialize Renderer
        m_renderer = std::make_unique<VulkanRenderer>(config);

        // 2. Extract handles for BufferManager
        VkDevice device = m_renderer->get_device();
        VkPhysicalDevice phys = m_renderer->get_physical_device();

        // 3. Initialize BufferManager
        m_buffer_manager = std::make_unique<BufferManager>(device, phys);
    }

    EngineContext::~EngineContext() {
        // Unique_ptrs handle cleanup automatically now that types are complete
    }

    bool EngineContext::is_running() const {
        return !m_renderer->should_close();
    }

    void EngineContext::new_frame() {
        m_renderer->poll_events();
        m_buffer_manager->reset_pools();
        m_renderer->begin_frame();
    }

    void EngineContext::end_frame() {
        m_renderer->end_frame();
    }
    void ndde::EngineContext::submit_frame() {
        m_renderer->end_frame();
    }

    void ndde::EngineContext::draw_geometry(const ArenaSlice& slice) {
        // Pass the command straight to the renderer
        m_renderer->record_draw(slice);
    }
} // namespace ndde