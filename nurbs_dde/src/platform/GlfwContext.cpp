// platform/GlfwContext.cpp
#include "platform/GlfwContext.hpp"

#include <volk.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <format>
#include <stdexcept>
#include <iostream>

namespace ndde::platform {

GlfwContext::~GlfwContext() { destroy(); }

void GlfwContext::init(u32 width, u32 height, const std::string& title) {
    if (!glfwInit())
        throw std::runtime_error("[GLFW] glfwInit() failed");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_TRUE);

    m_window = glfwCreateWindow(
        static_cast<int>(width), static_cast<int>(height),
        title.c_str(), nullptr, nullptr);

    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("[GLFW] glfwCreateWindow() failed");
    }

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);

    m_width.store(width,  std::memory_order_relaxed);
    m_height.store(height, std::memory_order_relaxed);
    m_initialised = true;

    std::cout << std::format("[GLFW] Window {}x{}: {}\n", width, height, title);
}

void GlfwContext::destroy() {
    if (!m_initialised) return;
    if (m_window) { glfwDestroyWindow(m_window); m_window = nullptr; }
    glfwTerminate();
    m_initialised = false;
}

void GlfwContext::poll_events() { glfwPollEvents(); }

bool GlfwContext::should_close() const noexcept {
    return glfwWindowShouldClose(m_window) != 0;
}

bool GlfwContext::check_resize() noexcept {
    return m_resized.exchange(false, std::memory_order_acq_rel);
}

void GlfwContext::framebuffer_resize_callback(GLFWwindow* win, int w, int h) {
    auto* self = static_cast<GlfwContext*>(glfwGetWindowUserPointer(win));
    if (!self) return;
    self->m_width.store(static_cast<u32>(w),  std::memory_order_relaxed);
    self->m_height.store(static_cast<u32>(h), std::memory_order_relaxed);
    self->m_resized.store(true, std::memory_order_release);
}

} // namespace ndde::platform
