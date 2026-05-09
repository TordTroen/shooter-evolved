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

set(SOURCES
    main.cpp
    src/core/Window.cpp
    src/rendering/GLDebug.cpp
    src/rendering/Shader.cpp
    src/rendering/Mesh.cpp
    src/scene/Camera.cpp
)
add_executable(fps_demo ${SOURCES})

target_include_directories(fps_demo PRIVATE src third_party)

find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_subdirectory(third_party/glad)

target_link_libraries(fps_demo PRIVATE SDL3::SDL3 glm::glm glad opengl32)
