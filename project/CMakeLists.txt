cmake_minimum_required(VERSION 3.20)

if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
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

set(SOURCES
    main.cpp
    src/actor/Actor.cpp
    src/actor/OscillatingActor.cpp
    src/core/Window.cpp
    src/rendering/GLDebug.cpp
    src/rendering/Shader.cpp
    src/rendering/Mesh.cpp
    src/scene/Camera.cpp
    src/physics/PhysicsWorld.cpp
    src/player/CharacterController.cpp
)
add_executable(fps_demo ${SOURCES})

target_include_directories(fps_demo PRIVATE
    src
    third_party
    ${JoltPhysics_SOURCE_DIR}/..
)

find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_subdirectory(third_party/glad)

target_link_libraries(fps_demo PRIVATE
    SDL3::SDL3
    glm::glm
    glad
    opengl32
    Jolt
)

option(BUILD_TESTING "Build unit tests" ON)

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
