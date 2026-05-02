#pragma once
// engine/Engine.hpp

#include "engine/AppConfig.hpp"
#include "engine/EngineAPI.hpp"
#include "engine/IScene.hpp"
#include "platform/GlfwContext.hpp"
#include "platform/VulkanContext.hpp"
#include "renderer/Swapchain.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/SecondWindow.hpp"
#include "memory/BufferManager.hpp"
#include <memory>
#include <string>
#include <functional>

namespace ndde {

class Scene;  // legacy forward-declaration (type still needed by Engine.cpp include)

class Engine {
public:
    using SceneFactory = std::function<std::unique_ptr<IScene>(EngineAPI)>;

    Engine();
    ~Engine();

    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;

    void start(const std::string& config_path = "engine_config.json");
    void run();

    // Replace the active scene with a new one.
    // Engine flushes the GPU before destroying the old scene.
    // factory is called with a fresh EngineAPI so the new scene is
    // fully initialised before the old one is destroyed.
    void switch_scene(SceneFactory factory);

    [[nodiscard]] const AppConfig& config() const noexcept { return m_config; }

private:
    AppConfig               m_config;
    platform::GlfwContext   m_glfw;
    platform::VulkanContext m_vk;
    renderer::Swapchain     m_swapchain;
    renderer::Renderer      m_renderer;
    memory::BufferManager   m_buffer_manager;
    std::unique_ptr<Scene>   m_scene;      ///< legacy slot (null — kept for compat)
    std::unique_ptr<IScene>  m_active;     ///< the live scene
    SceneFactory             m_pending_scene_switch;
    renderer::SecondWindow   m_second_win; ///< 2D contour window
    bool                     m_running     = false;

    // Per-frame state
    double     m_last_frame_time = 0.0;
    DebugStats m_debug_stats;

    void run_frame();
    void handle_resize();
    void apply_pending_scene_switch();
    [[nodiscard]] EngineAPI make_api();
};

} // namespace ndde
