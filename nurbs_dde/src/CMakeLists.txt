# src/CMakeLists.txt

# ── ndde_math ─────────────────────────────────────────────────────────────────
add_library(ndde_math STATIC
    math/Conics.cpp
    math/Surfaces.cpp
    math/Axes.cpp
)
target_include_directories(ndde_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(ndde_math PUBLIC VK_NO_PROTOTYPES)
target_include_directories(ndde_math PUBLIC ${volk_SOURCE_DIR})
target_link_libraries(ndde_math PUBLIC glm::glm Vulkan::Headers)

# ── ndde_memory ───────────────────────────────────────────────────────────────
add_library(ndde_memory STATIC
    memory/BufferManager.cpp
)
target_include_directories(ndde_memory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ndde_memory PUBLIC ndde_math volk)

# ── ndde_platform ─────────────────────────────────────────────────────────────
add_library(ndde_platform STATIC
    platform/GlfwContext.cpp
    platform/VulkanContext.cpp
)
target_include_directories(ndde_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ndde_platform PUBLIC ndde_math volk vk-bootstrap::vk-bootstrap glfw)

# ── ndde_renderer ─────────────────────────────────────────────────────────────
add_library(ndde_renderer STATIC
    renderer/Swapchain.cpp
    renderer/Pipeline.cpp
    renderer/ImGuiLayer.cpp
    renderer/Renderer.cpp
)
target_include_directories(ndde_renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ndde_renderer PUBLIC
    ndde_platform
    ndde_memory
    imgui
    Vulkan::Vulkan
)

# ── ndde_engine ───────────────────────────────────────────────────────────────
add_library(ndde_engine STATIC
    engine/AppConfig.cpp
    engine/Engine.cpp
)
target_include_directories(ndde_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ndde_engine PUBLIC
    ndde_renderer
    ndde_memory
    ndde_platform
    ndde_math
    nlohmann_json::nlohmann_json
)

# ── nurbs_dde executable ──────────────────────────────────────────────────────
add_executable(nurbs_dde
    app/main.cpp
    app/Scene.cpp
    app/AnalysisPanel.cpp
)
target_include_directories(nurbs_dde PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(nurbs_dde PRIVATE ndde_engine imgui)

# ── Compiler flags ────────────────────────────────────────────────────────────
set(NDDE_TARGETS ndde_math ndde_memory ndde_platform ndde_renderer ndde_engine nurbs_dde)

foreach(tgt IN LISTS NDDE_TARGETS)
    if(MSVC)
        target_compile_options(${tgt} PRIVATE
            /W4 /WX- /wd4100 /permissive- /utf-8
            $<$<CONFIG:Debug>:/Od /Zi /RTC1>
            $<$<CONFIG:Release>:/O2 /DNDEBUG>
        )
    else()
        target_compile_options(${tgt} PRIVATE
            -Wall -Wextra -Wpedantic -Wno-unused-parameter
            $<$<CONFIG:Debug>:-g -O0>
            $<$<CONFIG:Release>:-O3 -march=native>
        )
    endif()
endforeach()

# ── Shaders ───────────────────────────────────────────────────────────────────
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
compile_shaders(
    TARGET     nurbs_dde
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders"
    SHADERS
        "${SHADER_SOURCE_DIR}/curve.vert"
        "${SHADER_SOURCE_DIR}/curve.frag"
)

set(ASSETS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/assets")

target_compile_definitions(nurbs_dde  PRIVATE
    SHADER_DIR="${CMAKE_BINARY_DIR}/shaders"
    ASSETS_DIR="${ASSETS_OUTPUT_DIR}"
)
target_compile_definitions(ndde_engine PRIVATE
    SHADER_DIR="${CMAKE_BINARY_DIR}/shaders"
    ASSETS_DIR="${ASSETS_OUTPUT_DIR}"
)

# ── Assets copy ───────────────────────────────────────────────────────────────
add_custom_target(nurbs_dde_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets" "${ASSETS_OUTPUT_DIR}"
    COMMENT "Copying assets to build directory"
)
add_dependencies(nurbs_dde nurbs_dde_assets)

# ── engine_config.json copy ───────────────────────────────────────────────────
add_custom_target(nurbs_dde_config ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/engine_config.json"
        "${CMAKE_BINARY_DIR}/engine_config.json"
    COMMENT "Copying engine_config.json to build directory"
)
add_dependencies(nurbs_dde nurbs_dde_config)
