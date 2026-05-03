# tests/CMakeLists.txt
enable_testing()

add_executable(ndde_tests
    test_all_simulations.cpp
    test_conics.cpp
    test_axes.cpp
    test_numeric.cpp
    test_particle_system.cpp
    test_surface_mesh_cache.cpp
    test_engine_services.cpp
    test_simulation_context.cpp
    test_simulation_runtime.cpp
    test_simulation_surface_gaussian.cpp
    ${CMAKE_SOURCE_DIR}/src/app/GaussianSurface.cpp
    ${CMAKE_SOURCE_DIR}/src/app/ParticleSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/app/AnalysisSpawner.cpp
    ${CMAKE_SOURCE_DIR}/src/app/SceneFactories.cpp
    ${CMAKE_SOURCE_DIR}/src/app/MultiWellSpawner.cpp
    ${CMAKE_SOURCE_DIR}/src/app/SimulationAnalysis.cpp
    ${CMAKE_SOURCE_DIR}/src/app/SimulationMultiWell.cpp
    ${CMAKE_SOURCE_DIR}/src/app/SimulationSurfaceGaussian.cpp
    ${CMAKE_SOURCE_DIR}/src/app/SimulationWavePredatorPrey.cpp
    ${CMAKE_SOURCE_DIR}/src/app/SurfaceMeshCache.cpp
    ${CMAKE_SOURCE_DIR}/src/app/SurfaceSimSpawner.cpp
    ${CMAKE_SOURCE_DIR}/src/app/WavePredatorPreySpawner.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/SimulationRuntime.cpp
)

target_include_directories(ndde_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(ndde_tests PRIVATE
    ndde_math
    ndde_numeric
    volk
    imgui
    GTest::gtest_main
    glm::glm
)

if(MSVC)
    target_compile_options(ndde_tests PRIVATE
        /W4 /WX- /wd4100 /permissive-
        $<$<CONFIG:Debug>:/Od /Zi /RTC1>
        $<$<CONFIG:Release>:/O2>
    )
else()
    target_compile_options(ndde_tests PRIVATE
        -Wall -Wextra -Wpedantic -Wno-unused-parameter
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3>
        $<$<AND:$<CONFIG:Debug>,$<BOOL:${ENABLE_SANITIZERS}>>:-fsanitize=address,undefined>
    )
    target_link_options(ndde_tests PRIVATE
        $<$<AND:$<CONFIG:Debug>,$<BOOL:${ENABLE_SANITIZERS}>>:-fsanitize=address,undefined>
    )
endif()

include(GoogleTest)
gtest_discover_tests(ndde_tests)

add_test(
    NAME allocation_policy
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/check_allocation_policy.py
)
