add_executable(fps_demo_tests
    test_basic.cpp
    test_primitives.cpp
    test_procedural_textures.cpp
    test_paths.cpp
    test_hud.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/Primitives.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/ProceduralTextures.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Paths.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Hud.cpp
)

target_include_directories(fps_demo_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(fps_demo_tests PRIVATE
    Catch2::Catch2WithMain
    SDL3::SDL3
    imgui::imgui
    glm::glm
)

include(Catch)
catch_discover_tests(fps_demo_tests)
