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
    ${CMAKE_SOURCE_DIR}/src/rendering/Primitives.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/ProceduralTextures.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Paths.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Args.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Hud.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ActorState.cpp
    ${CMAKE_SOURCE_DIR}/src/net/BitStream.cpp
    ${CMAKE_SOURCE_DIR}/src/net/Endpoint.cpp
    ${CMAKE_SOURCE_DIR}/src/net/InputFrame.cpp
    ${CMAKE_SOURCE_DIR}/src/net/PlayerState.cpp
    ${CMAKE_SOURCE_DIR}/src/net/FireIntent.cpp
    ${CMAKE_SOURCE_DIR}/src/net/Snapshot.cpp
    ${CMAKE_SOURCE_DIR}/src/net/LobbyRoster.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NameGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/net/MockTransport.cpp
    ${CMAKE_SOURCE_DIR}/src/scene/Orientation.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Scoreboard.cpp
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
