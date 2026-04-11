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
        // --- 1. Initialize the Service Locator ---
        // This creates our AppConfig (2D/3D flag), Renderer, and BufferManager.
        // By using unique_ptr, we guarantee destruction order when main() exits.
        auto context = std::make_unique<ndde::EngineContext>();

        // --- 2. Initialize the Application ---
        // We move ownership of the context to the application.
        // The Application will now own the "Heart" of the simulation.
        ndde::Application app(std::move(context));

        // --- 3. Run the Simulation Loop ---
        // This is where the DDE pursuit logic and Vulkan draw calls live.
        app.run();

    } catch (const std::exception& e) {
        // In a high-performance simulation, we must catch startup
        // failures (like Vulkan device loss) immediately.
        std::cerr << "Fatal Error during Engine Execution: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}