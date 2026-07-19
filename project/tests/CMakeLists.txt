add_executable(fps_demo_tests
    test_basic.cpp
    test_primitives.cpp
    test_procedural_textures.cpp
    test_paths.cpp
    test_hud.cpp
    test_net.cpp
    test_network_id.cpp
    test_args.cpp
    test_endpoint.cpp
    test_orientation.cpp
    test_decals.cpp
    test_scoreboard.cpp
    test_weapons.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/Primitives.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/ProceduralTextures.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Hud.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Scoreboard.cpp
)

target_link_libraries(fps_demo_tests PRIVATE
    Catch2::Catch2WithMain
    fps_core
    imgui::imgui
)

include(Catch)
catch_discover_tests(fps_demo_tests)
