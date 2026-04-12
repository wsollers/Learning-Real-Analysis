set(APP_SOURCES
    app/main.cpp
    app/Application.cpp
    renderer/VulkanContext.cpp
    renderer/Swapchain.cpp
    renderer/Pipeline.cpp
    renderer/ImGuiLayer.cpp
    geometry/Curve.cpp
    geometry/Parabola.cpp
    geometry/Hyperbola.cpp
    geometry/Axes2D.cpp
    math/Vec3.cpp
        app/AnalysisPanel.hpp
        app/AnalysisPanel.cpp
        app/HoverResult.hpp
        geometry/ICurve.hpp
        core/Types.hpp
        core/EngineContext.hpp
        app/AppConfig.hpp
        core/EngineContext.cpp
        renderer/VulkanRenderer.hpp
        renderer/VulkanRenderer.cpp
        core/BufferManager.hpp
        core/BufferManager.cpp
        geometry/ICurve.cpp
)

add_executable(nurbs_dde ${APP_SOURCES})

target_include_directories(nurbs_dde PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(nurbs_dde
    PRIVATE
        vk-bootstrap::vk-bootstrap
        Vulkan::Vulkan
        glfw
        glm::glm
        imgui
)

if(MSVC)
    target_compile_options(nurbs_dde PRIVATE
        /W4 /WX- /wd4100 /permissive- /utf-8
        $<$<CONFIG:Debug>:/Od /Zi /RTC1>
        $<$<CONFIG:Release>:/O2 /DNDEBUG>
    )
else()
    target_compile_options(nurbs_dde PRIVATE
        -Wall -Wextra -Wpedantic -Wno-unused-parameter
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -march=native>
        $<$<AND:$<CONFIG:Debug>,$<BOOL:${ENABLE_SANITIZERS}>>:-fsanitize=address,undefined>
    )
    target_link_options(nurbs_dde PRIVATE
        $<$<AND:$<CONFIG:Debug>,$<BOOL:${ENABLE_SANITIZERS}>>:-fsanitize=address,undefined>
    )
endif()

set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
compile_shaders(
    TARGET     nurbs_dde
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders"
    SHADERS
        "${SHADER_SOURCE_DIR}/curve.vert"
        "${SHADER_SOURCE_DIR}/curve.frag"
)

set(ASSETS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/assets")

add_custom_target(nurbs_dde_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${ASSETS_OUTPUT_DIR}"
        COMMENT "Copying assets to build directory"
)
add_dependencies(nurbs_dde nurbs_dde_assets)

target_compile_definitions(nurbs_dde PRIVATE
        ASSETS_DIR="${ASSETS_OUTPUT_DIR}"
)
