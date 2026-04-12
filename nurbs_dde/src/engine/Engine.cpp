// engine/Engine.cpp
#define VOLK_IMPLEMENTATION
#include <volk.h>

#include "engine/Engine.hpp"
#include "app/Scene.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <format>
#include <iostream>
#include <stdexcept>

namespace ndde {

Engine::Engine() = default;

Engine::~Engine() {
    m_scene.reset();
    m_renderer.destroy();
    m_buffer_manager.destroy();
    m_swapchain.destroy();
    m_vk.destroy();
    m_glfw.destroy();
}

void Engine::start(const std::string& config_path) {
    std::cout << "[Engine] Starting...\n";

    m_config = AppConfig::load_or_default(config_path);

    m_glfw.init(m_config.window.width,
                m_config.window.height,
                m_config.window.title);

    if (volkInitialize() != VK_SUCCESS)
        throw std::runtime_error("[Engine] volkInitialize() failed");

    m_vk.init(m_glfw.window(), m_config.window.title);
    m_swapchain.init(m_vk, m_glfw.width(), m_glfw.height());

    // ASSETS_DIR is an absolute path injected by CMake at compile time —
    // identical to how SHADER_DIR works. This means the exe finds its fonts
    // regardless of which working directory CLion (or the shell) launches from.
    m_renderer.init(m_vk, m_swapchain, SHADER_DIR, ASSETS_DIR, m_glfw.window());

    m_buffer_manager.init(m_vk.device(), m_vk.physical_device(),
                          m_config.simulation.arena_size_mb);
    m_scene = std::make_unique<Scene>(make_api());

    m_running = true;
    std::cout << "[Engine] Ready.\n";
}

void Engine::run() {
    while (m_running && !m_glfw.should_close()) {
        m_glfw.poll_events();
        if (m_glfw.check_resize()) { handle_resize(); continue; }
        run_frame();
    }
    vkDeviceWaitIdle(m_vk.device());
}

void Engine::run_frame() {
    m_buffer_manager.reset();
    if (!m_renderer.begin_frame(m_swapchain)) { handle_resize(); return; }
    m_renderer.imgui_new_frame();
    m_scene->on_frame();
    m_renderer.imgui_render();
    if (!m_renderer.end_frame(m_swapchain)) handle_resize();
}

void Engine::handle_resize() {
    const u32 w = m_glfw.width();
    const u32 h = m_glfw.height();
    if (w == 0 || h == 0) return;
    vkDeviceWaitIdle(m_vk.device());
    m_swapchain.recreate(m_vk, w, h);
    m_renderer.on_swapchain_recreated(m_swapchain);
    std::cout << std::format("[Engine] Swapchain {}x{}\n", w, h);
}

EngineAPI Engine::make_api() {
    EngineAPI api;

    api.acquire = [this](u32 n) -> memory::ArenaSlice {
        return m_buffer_manager.acquire(n);
    };

    api.submit = [this](const memory::ArenaSlice& slice,
                         Topology topology, DrawMode mode, Vec4 color, Mat4 mvp) {
        m_renderer.draw(renderer::DrawCall{
            .slice = slice, .topology = topology,
            .mode  = mode,  .color    = color, .mvp = mvp
        });
    };

    api.math_font_body  = [this]() -> ImFont* { return m_renderer.font_math_body();  };
    api.math_font_small = [this]() -> ImFont* { return m_renderer.font_math_small(); };
    api.config          = [this]() -> const AppConfig& { return m_config; };
    api.viewport_size   = [this]() -> Vec2 {
        return Vec2{ static_cast<f32>(m_glfw.width()),
                     static_cast<f32>(m_glfw.height()) };
    };

    return api;
}

} // namespace ndde
