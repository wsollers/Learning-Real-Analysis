// app/main.cpp
#include "core/EngineContext.hpp"
#include "app/Application.hpp"
#include <iostream>
#include <memory>

#define VOLK_IMPLEMENTATION
#include <volk.h>

int main() {
    try {
        // 0 - Initialize VOLK
        if (volkInitialize() != VK_SUCCESS) {
            throw std::runtime_error("Failed to initialize Volk");
        }
        ndde::AppConfig config;
        // ... setup config ...

        // Create the context first
        auto context = std::make_unique<ndde::EngineContext>(config);

        // Pass the context into Application
        ndde::Application app(std::move(context));
        app.run();


    } catch (const std::exception& e) {
        // In a high-performance simulation, we must catch startup
        // failures (like Vulkan device loss) immediately.
        std::cerr << "Fatal Error during Engine Execution: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}