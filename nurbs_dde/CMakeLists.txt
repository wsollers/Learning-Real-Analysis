cmake_minimum_required(VERSION 3.27)
project(nurbs_dde VERSION 0.2.0 LANGUAGES CXX)

# ── C++23 ─────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler guard ────────────────────────────────────────────────────────────
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(WARNING "This project targets Clang. Current: ${CMAKE_CXX_COMPILER_ID}")
endif()

# ── Build type default ────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ── Options ───────────────────────────────────────────────────────────────────
option(ENABLE_SANITIZERS "Build with ASan + UBSan (Debug only)" ON)

# ── Module path ───────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ── Dependencies ──────────────────────────────────────────────────────────────
include(FetchContent)

# vk-bootstrap
FetchContent_Declare(vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
    GIT_TAG        v1.3.295)
FetchContent_MakeAvailable(vk_bootstrap)

# Volk
FetchContent_Declare(volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG        1.3.295)
FetchContent_MakeAvailable(volk)

# GLFW
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# GLM (header-only)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1)
FetchContent_MakeAvailable(glm)

# nlohmann/json (header-only, for engine_config.json loading)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# Google Test
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Dear ImGui (docking branch, no CMakeLists — we define the target manually)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    OFF)
FetchContent_MakeAvailable(imgui)

find_package(Vulkan REQUIRED)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan volk glfw)

# ── Global Vulkan define ──────────────────────────────────────────────────────
# volk loads all Vulkan function pointers at runtime.
# VK_NO_PROTOTYPES tells every Vulkan header not to declare the standard
# extern symbols, preventing duplicate-symbol linker errors.
add_compile_definitions(VK_NO_PROTOTYPES)

# ── Shader compilation ────────────────────────────────────────────────────────
include(CompileShaders)

# ── Subdirectories ────────────────────────────────────────────────────────────
add_subdirectory(src)
add_subdirectory(tests)
