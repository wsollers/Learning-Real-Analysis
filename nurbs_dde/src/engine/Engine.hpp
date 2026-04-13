#pragma once
// engine/Engine.hpp

#include "engine/AppConfig.hpp"
#include "engine/EngineAPI.hpp"
#include "platform/GlfwContext.hpp"
#include "platform/VulkanContext.hpp"
#include "renderer/Swapchain.hpp"
#include "renderer/Renderer.hpp"
#include "memory/BufferManager.hpp"
#include <memory>
#include <string>

namespace ndde {

class Scene;

class Engine {
public:
    Engine();
    ~Engine();

    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;

    void start(const std::string& config_path = "engine_config.json");
    void run();

    [[nodiscard]] const AppConfig& config() const noexcept { return m_config; }

private:
    AppConfig               m_config;
    platform::GlfwContext   m_glfw;
    platform::VulkanContext m_vk;
    renderer::Swapchain     m_swapchain;
    renderer::Renderer      m_renderer;
    memory::BufferManager   m_buffer_manager;
    std::unique_ptr<Scene>  m_scene;
    bool                    m_running       = false;

    // ── Per-frame state ───────────────────────────────────────────────────────
    double     m_last_frame_time = 0.0;  ///< glfwGetTime() of previous frame
    DebugStats m_debug_stats;            ///< populated each frame, exposed via EngineAPI

    void run_frame();
    void handle_resize();
    [[nodiscard]] EngineAPI make_api();
};

} // namespace ndde
