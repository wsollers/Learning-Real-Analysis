#include "app/Application.hpp"
#include "app/AppConfig.hpp"
#include "renderer/VulkanRenderer.hpp"
#include <GLFW/glfw3.h> // for glfwGetTime

namespace ndde {

    Application::Application(std::unique_ptr<EngineContext> context)
       : m_context(std::move(context))
    {
        // Constructor body can be empty now
    }

    void Application::run() {
        if (!m_context) return;

        while (m_context->is_running()) {
            m_context->new_frame();

            // 1. Get the slice (the metadata)
            auto slice = m_context->acquire_geometry_slice(3);

            // 2. Get the actual memory pointer and tell the compiler it's a Vertex array
            Vertex* vram = static_cast<Vertex*>(slice.data);

            // 3. Write to the pointer, NOT the slice object
            vram[0] = { {  0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }; // Red top
            vram[1] = { {  0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } }; // Green right
            vram[2] = { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }; // Blue left
            m_context->draw_geometry(slice);
            m_context->submit_frame();
        }
    }

    Application::~Application() {
        // You can leave this empty.
        // The unique_ptr<EngineContext> will clean itself up automatically!
    }
} // namespace ndde