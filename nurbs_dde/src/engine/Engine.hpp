#pragma once
// engine/Engine.hpp

#include "engine/AppConfig.hpp"
#include "engine/EngineAPI.hpp"
#include "engine/IScene.hpp"
#include "engine/SimulationRuntime.hpp"
#include "platform/GlfwContext.hpp"
#include "platform/VulkanContext.hpp"
#include "renderer/Swapchain.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/SecondWindow.hpp"
#include "memory/BufferManager.hpp"
#include <memory>
#include <filesystem>
#include <string>
#include <functional>

namespace ndde {

void engine_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

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
    void switch_simulation(std::size_t index);

    [[nodiscard]] const AppConfig& config() const noexcept { return m_config; }

private:
    AppConfig               m_config;
    platform::GlfwContext   m_glfw;
    platform::VulkanContext m_vk;
    renderer::Swapchain     m_swapchain;
    renderer::Renderer      m_renderer;
    memory::BufferManager   m_buffer_manager;
    std::unique_ptr<Scene>   m_scene;      ///< legacy slot (null — kept for compat)
    SimulationRegistry       m_simulations;
    std::size_t              m_active_sim = 0;
    std::size_t              m_pending_sim = static_cast<std::size_t>(-1);
    std::unique_ptr<IScene>  m_active;     ///< legacy live scene slot (unused for registered simulations)
    SceneFactory             m_pending_scene_switch;
    renderer::SecondWindow   m_second_win; ///< 2D contour window
    bool                     m_running     = false;
    bool                     m_hotkey_sim1_prev = false;
    bool                     m_hotkey_sim2_prev = false;
    bool                     m_hotkey_capture_prev = false;
    bool                     m_hotkey_pause_capture_prev = false;

    // Per-frame state
    double     m_last_frame_time = 0.0;
    DebugStats m_debug_stats;

    void run_frame();
    void handle_resize();
    void apply_pending_scene_switch();
    void apply_pending_simulation_switch();
    void draw_global_panels();
    void install_global_hotkeys();
    void uninstall_global_hotkeys() noexcept;
    void on_key_event(int key, int action, int mods);
    void dispatch_global_hotkeys();
    void request_capture(bool pause_first);
    [[nodiscard]] std::filesystem::path make_capture_path() const;
    [[nodiscard]] SceneSimulationRuntime& active_runtime();
    [[nodiscard]] const SceneSimulationRuntime& active_runtime() const;
    [[nodiscard]] EngineAPI make_api();

    friend void engine_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

} // namespace ndde
