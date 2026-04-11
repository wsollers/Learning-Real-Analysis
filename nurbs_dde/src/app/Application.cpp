#include "app/Application.hpp"
#include "app/AppConfig.hpp"
#include "renderer/VulkanRenderer.hpp"
#include <GLFW/glfw3.h> // for glfwGetTime

namespace ndde {

    Application::Application(std::unique_ptr<EngineContext> context)
        : m_ctx(std::move(context))
    {
        m_last_time = glfwGetTime();
        // init_curves logic will eventually move to a GeometryManager
        // or be handled in update() based on the config.
    }

    Application::~Application() {
        // Context handles cleanup of renderer/window/vulkan automatically
    }

    void Application::run() {
        // Instead of m_window, we ask the renderer if we should close
        while (!m_ctx->renderer->should_close()) {
            m_ctx->renderer->poll_events();

            update();
            render();
        }
    }

    void Application::update() {
        double current_time = glfwGetTime();
        double delta_time = current_time - m_last_time;
        m_last_time = current_time;

        // Any DDE simulation logic goes here, using m_ctx->config

    }

    void Application::render() {
        // The Renderer handles the frame lifecycle
        m_ctx->renderer->begin_frame();

        // Pass the config so the renderer knows about 2D/3D toggle, zoom, etc.
        m_ctx->renderer->draw_geometry(*m_ctx->config);

        m_ctx->renderer->end_frame();
    }

} // namespace ndde