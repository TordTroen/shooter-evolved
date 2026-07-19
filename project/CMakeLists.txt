cmake_minimum_required(VERSION 3.20)

if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# Must be set before project() — it drives which vcpkg manifest features
# (sdl3/imgui/stb, the "client" feature) get installed by the toolchain file.
option(BUILD_CLIENT "Build windowed fps_demo client" ${CMAKE_HOST_WIN32})
if(NOT BUILD_CLIENT)
    set(VCPKG_MANIFEST_NO_DEFAULT_FEATURES ON CACHE BOOL "")
endif()

project(fps_demo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Jolt feature flags — must be set BEFORE FetchContent_MakeAvailable
# so they propagate into Jolt's own build. These same defines must be
# visible to game code that includes Jolt headers (handled below via
# add_compile_definitions, which applies project-wide).
add_compile_definitions(
    JPH_NO_FORCE_INLINE       # Avoid __forceinline issues if LTO is enabled
    JPH_DEBUG_RENDERER        # Enable debug shape rendering hooks
    JPH_PROFILE_ENABLED       # Enable internal profiler
)

FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        v5.5.0
    SOURCE_SUBDIR  "Build"
)
# Match vcpkg's dynamic CRT (/MD) so the linker doesn't see a MT/MD mismatch.
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(JoltPhysics)

find_package(glm CONFIG REQUIRED)
find_package(GameNetworkingSockets CONFIG REQUIRED)

if(BUILD_CLIENT)
    find_package(SDL3 CONFIG REQUIRED)
    find_package(imgui CONFIG REQUIRED)
    find_package(Stb REQUIRED)
endif()

# ---------------------------------------------------------------------------
# fps_core — graphics-free static library shared by fps_demo, fps_server, and
# the test suite. No SDL/GL/ImGui here; Paths.cpp resolves the executable
# directory with portable OS calls, so fps_core needs no graphics stack.
# ---------------------------------------------------------------------------
set(CORE_SOURCES
    src/actor/Actor.cpp
    src/actor/OscillatingActor.cpp
    src/actor/SpawnPoint.cpp
    src/actor/components/PhysicsBody.cpp
    src/core/Args.cpp
    src/core/Paths.cpp
    src/net/ActorState.cpp
    src/net/BitStream.cpp
    src/net/Client.cpp
    src/net/Endpoint.cpp
    src/net/FireIntent.cpp
    src/net/GnsTransport.cpp
    src/net/InputFrame.cpp
    src/net/LobbyRoster.cpp
    src/net/MockTransport.cpp
    src/net/NameGenerator.cpp
    src/net/Net.cpp
    src/net/PlayerState.cpp
    src/net/Server.cpp
    src/net/Snapshot.cpp
    src/net/SpawnSelector.cpp
    src/physics/PhysicsWorld.cpp
    src/player/CharacterController.cpp
    src/player/Weapon.cpp
    src/scene/DemoScene.cpp
    src/scene/Orientation.cpp
    src/scene/Scene.cpp
    src/util/CodeTimer.cpp
    src/weapons/Pellets.cpp
    src/weapons/WeaponRegistry.cpp
)
add_library(fps_core STATIC ${CORE_SOURCES})

target_include_directories(fps_core PUBLIC
    src
    third_party
    ${JoltPhysics_SOURCE_DIR}/..
)

target_compile_options(fps_core PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Werror>
)

target_link_libraries(fps_core PUBLIC
    glm::glm
    Jolt
    GameNetworkingSockets::GameNetworkingSockets
)

# ---------------------------------------------------------------------------
# fps_demo — the windowed game: fps_core + rendering/ui/input/Window/Game.
# Windows-only for now (opengl32 import lib name is non-portable); gated
# behind BUILD_CLIENT, which defaults ON on Windows and OFF elsewhere.
# ---------------------------------------------------------------------------
if(BUILD_CLIENT)
    target_link_libraries(fps_core PUBLIC SDL3::SDL3)

    set(DEMO_SOURCES
        main.cpp
        src/actor/components/MeshRenderer.cpp
        src/core/Game.cpp
        src/core/Window.cpp
        src/input/GamepadInput.cpp
        src/input/InputFrameLocal.cpp
        src/rendering/GLDebug.cpp
        src/rendering/Mesh.cpp
        src/rendering/ModelLoader.cpp
        src/rendering/DecalRenderer.cpp
        src/rendering/MuzzleFlashEffect.cpp
        src/rendering/Primitives.cpp
        src/rendering/ProceduralTextures.cpp
        src/rendering/RemotePlayerRenderer.cpp
        src/rendering/Shader.cpp
        src/rendering/Texture.cpp
        src/rendering/ViewmodelRenderer.cpp
        src/scene/Camera.cpp
        src/state/LobbyState.cpp
        src/state/MainMenuState.cpp
        src/state/PlayingState.cpp
        src/ui/Hud.cpp
        src/ui/ImGuiLayer.cpp
        src/ui/Scoreboard.cpp
    )
    add_executable(fps_demo ${DEMO_SOURCES})

    add_subdirectory(third_party/glad)

    target_include_directories(fps_demo PRIVATE ${Stb_INCLUDE_DIR})

    target_link_libraries(fps_demo PRIVATE
        fps_core
        imgui::imgui
        glad
        opengl32
    )
endif()

# ---------------------------------------------------------------------------
# fps_server — command-line-only dedicated server: fps_core + server entry.
# No SDL video/GL/ImGui links; runs headless on machines with no GPU/display.
# ---------------------------------------------------------------------------
set(SERVER_SOURCES
    src/server/server_main.cpp
    src/server/HeadlessServer.cpp
)
add_executable(fps_server ${SERVER_SOURCES})

target_link_libraries(fps_server PRIVATE fps_core)

option(BUILD_TESTING "Build unit tests" ${BUILD_CLIENT})

if(BUILD_TESTING)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.8.1
    )
    FetchContent_MakeAvailable(Catch2)

    enable_testing()
    add_subdirectory(tests)
endif()
