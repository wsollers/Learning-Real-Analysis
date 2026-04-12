#pragma once
// engine/Engine.hpp
// Single assembly point for all subsystems.
//
// Usage:
//   Engine engine;
//   engine.start();   // loads engine_config.json, wires subsystems
//   engine.run();     // blocks until window close
//   // RAII cleanup in destructor
//
// MSVC note: ~Engine() is declared here but defined in Engine.cpp.
// This is required because Engine holds unique_ptr<Scene> and Scene is
// forward-declared here. unique_ptr's destructor requires a complete type,
// so the destructor body must live in a TU that includes Scene.hpp.

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

class Scene;   // forward declaration — complete type in Engine.cpp

class Engine {
public:
    Engine();   // defined in Engine.cpp
    ~Engine();  // defined in Engine.cpp — Scene must be complete at that point

    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;

    // Load config, create window, init Vulkan, wire everything together.
    void start(const std::string& config_path = "engine_config.json");

    // Frame loop — blocks until window close.
    void run();

    [[nodiscard]] const AppConfig& config() const noexcept { return m_config; }

private:
    AppConfig               m_config;
    platform::GlfwContext   m_glfw;
    platform::VulkanContext m_vk;
    renderer::Swapchain     m_swapchain;
    renderer::Renderer      m_renderer;
    memory::BufferManager   m_buffer_manager;
    std::unique_ptr<Scene>  m_scene;   // complete type required only at destruction
    bool                    m_running = false;

    void run_frame();
    void handle_resize();
    [[nodiscard]] EngineAPI make_api();
};

} // namespace ndde
