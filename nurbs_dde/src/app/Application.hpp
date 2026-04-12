#pragma once
// app/Application.hpp

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "renderer/VulkanContext.hpp"
#include "renderer/Swapchain.hpp"
#include "app/CurveEntry.hpp"
#include "app/AnalysisPanel.hpp"
#include "math/Vec3.hpp"

#include <vector>
#include <memory>

#include "core/Types.hpp"

#pragma once
#include "core/EngineContext.hpp"
#include <memory>

namespace ndde {
    class Application {
    public:
        explicit Application(std::unique_ptr<EngineContext> context);
        ~Application();

        void run();

    private:
        void update();

        std::unique_ptr<EngineContext> m_context;
        double m_last_time = 0.0;
    };
}