#pragma once
// engine/Engine.hpp

#include "engine/AppConfig.hpp"
#include "engine/EngineAPI.hpp"
#include "engine/IScene.hpp"
#include "engine/SimulationHost.hpp"
#include "engine/SimulationRuntime.hpp"
#include "platform/GlfwContext.hpp"
#include "platform/VulkanContext.hpp"
#include "renderer/Swapchain.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/SecondWindow.hpp"
#include "memory/BufferManager.hpp"
#include "memory/Containers.hpp"
#include <memory>
#include <filesystem>
#include <string>

namespace ndde {

void engine_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

class Engine {
public:
    Engine();
    ~Engine();

    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;

    void start(const std::string& config_path = "engine_config.json");
    void run();

    void switch_simulation(std::size_t index);

    [[nodiscard]] const AppConfig& config() const noexcept { return m_config; }
    [[nodiscard]] EngineServices& services() noexcept { return m_services; }
    [[nodiscard]] const EngineServices& services() const noexcept { return m_services; }
    [[nodiscard]] PanelService& getPanelService() noexcept { return m_services.panels(); }
    [[nodiscard]] HotkeyService& getHotkeyService() noexcept { return m_services.hotkeys(); }
    [[nodiscard]] InteractionService& getInteractionService() noexcept { return m_services.interaction(); }
    [[nodiscard]] RenderService& getRenderService() noexcept { return m_services.render(); }
    [[nodiscard]] SimulationClock& getSimulationClock() noexcept { return m_services.clock(); }

private:
    AppConfig               m_config;
    EngineServices          m_services;
    SimulationHost          m_simulation_host;
    platform::GlfwContext   m_glfw;
    platform::VulkanContext m_vk;
    renderer::Swapchain     m_swapchain;
    renderer::Renderer      m_renderer;
    memory::BufferManager   m_buffer_manager;
    SimulationRegistry       m_simulations;
    std::size_t              m_active_sim = 0;
    std::size_t              m_pending_sim = static_cast<std::size_t>(-1);
    renderer::SecondWindow   m_second_win; ///< 2D contour window
    bool                     m_running     = false;

    // Per-frame state
    double     m_last_frame_time = 0.0;
    DebugStats m_debug_stats;
    u32        m_surface_perturb_seed = 1;
    memory::PersistentVector<PanelHandle> m_global_panels;
    memory::PersistentVector<std::string> m_event_log;

    void run_frame();
    void handle_resize();
    void apply_pending_simulation_switch();
    void register_global_panels();
    void draw_global_status_panel();
    void draw_debug_coordinates_panel();
    void draw_simulation_metadata_panel();
    void draw_event_log_panel();
    void flush_render_service();
    void install_global_hotkeys();
    void uninstall_global_hotkeys() noexcept;
    void on_key_event(int key, int action, int mods);
    void dispatch_global_hotkeys();
    void update_render_view_input();
    void request_capture(bool pause_first);
    [[nodiscard]] std::filesystem::path make_capture_path() const;
    [[nodiscard]] SimulationRuntime& active_runtime();
    [[nodiscard]] const SimulationRuntime& active_runtime() const;
    [[nodiscard]] EngineAPI make_api();

    friend void engine_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

} // namespace ndde
