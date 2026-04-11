enable_testing()

# Re-compile only the geometry + math TUs — no Vulkan/GLFW needed for unit tests.
add_executable(nurbs_dde_tests
    test_parabola.cpp
    test_curve.cpp
    test_vec3.cpp

    ${CMAKE_SOURCE_DIR}/src/geometry/Curve.cpp
    ${CMAKE_SOURCE_DIR}/src/geometry/Parabola.cpp
    ${CMAKE_SOURCE_DIR}/src/math/Vec3.cpp
)

target_include_directories(nurbs_dde_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(nurbs_dde_tests
    PRIVATE
        GTest::gtest_main
        glm::glm
)

# ── Compile flags — compiler-aware, applied to OUR target only ────────────────
if(MSVC)
    target_compile_options(nurbs_dde_tests PRIVATE
        /W4
        /WX-
        /wd4100
        /permissive-
        $<$<CONFIG:Debug>:/Od /Zi /RTC1>
        $<$<CONFIG:Release>:/O2>
    )
else()
    target_compile_options(nurbs_dde_tests PRIVATE
        -Wall -Wextra -Wpedantic -Wno-unused-parameter
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3>
        $<$<AND:$<CONFIG:Debug>,$<BOOL:${ENABLE_SANITIZERS}>>:-fsanitize=address,undefined>
    )
    target_link_options(nurbs_dde_tests PRIVATE
        $<$<AND:$<CONFIG:Debug>,$<BOOL:${ENABLE_SANITIZERS}>>:-fsanitize=address,undefined>
    )
endif()

include(GoogleTest)
gtest_discover_tests(nurbs_dde_tests)
