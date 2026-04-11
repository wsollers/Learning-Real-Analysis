cmake_minimum_required(VERSION 3.27)
project(nurbs_dde VERSION 0.1.0 LANGUAGES CXX)

# ── C++23 ──────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler: enforce Clang ────────────────────────────────────────────────────
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(WARNING "This project is designed for Clang. Current: ${CMAKE_CXX_COMPILER_ID}")
endif()

# ── Build type default ─────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ── Sanitizer toggle (our targets only) ───────────────────────────────────────
option(ENABLE_SANITIZERS "Build with ASan + UBSan (Debug only)" ON)

# ── Module path ────────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ── Dependencies ──────────────────────────────────────────────────────────────
include(FetchContent)

# vk-bootstrap
FetchContent_Declare(
        vk_bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
        GIT_TAG        v1.3.295
)
FetchContent_MakeAvailable(vk_bootstrap)

# Volk (must match your Vulkan SDK / vk-bootstrap version)
FetchContent_Declare(
        volk
        GIT_REPOSITORY https://github.com/zeux/volk.git
        GIT_TAG        1.3.295
)
FetchContent_MakeAvailable(volk)

# GLFW
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# GLM (header-only)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

# Google Test
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ── Dear ImGui (docking branch) ────────────────────────────────────────────────
# imgui ships no CMakeLists.txt — we fetch the sources and define the target
# ourselves below, after FetchContent populates imgui_SOURCE_DIR.
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        docking
        GIT_SHALLOW    OFF
)
FetchContent_MakeAvailable(imgui)

# Build imgui as a static library.
# We include only the backends we need: GLFW and Vulkan.
# All other backends (SDL, OpenGL, DX, Metal, …) are excluded.
add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp          # remove once you no longer need the demo
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)

# imgui's Vulkan backend needs the Vulkan headers and GLFW headers.
# Link glfw so the GLFW backend can find glfw3.h without an explicit -I.
find_package(Vulkan REQUIRED)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan volk glfw)

# imgui_impl_vulkan.h uses dynamic dispatch by default (loads vkGetInstanceProcAddr
# at runtime).  Define IMGUI_IMPL_VULKAN_NO_PROTOTYPES only if you want to load
# everything yourself — leave it undefined for the simple path.

# ── Vulkan (system) ────────────────────────────────────────────────────────────
# find_package already ran above; kept here as a no-op reminder so the variable
# is in scope for sub-directories.

# ── Shader compilation ─────────────────────────────────────────────────────────
include(CompileShaders)

# Volk is a meta-loader for Vulkan function pointers.  It requires the caller to
# define VK_NO_PROTOTYPES before including the Vulkan headers, so that it can load
# all function pointers at runtime instead of relying on static linking.
add_compile_definitions(VK_NO_PROTOTYPES)

# ── Sub-targets ───────────────────────────────────────────────────────────────
add_subdirectory(src)
add_subdirectory(tests)