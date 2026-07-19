#!/bin/sh
# Builds fps_server (headless dedicated server) on Linux.
# fps_demo (windowed client) stays Windows-only; see project/ai_plans/LinuxDedicatedServer.md.
set -e

cd "$(dirname "$0")"

BUILD_DIR=cmake-build-debug-linux
BUILD_TYPE=Debug

if [ "$1" = "release" ]; then
    BUILD_TYPE=Release
fi

echo "[build] Configuring CMake ${BUILD_TYPE}..."
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_CLIENT=OFF -DBUILD_TESTING=OFF

echo "[build] Building..."
cmake --build "$BUILD_DIR" --target fps_server

echo "[build] Done. Executable: ${BUILD_DIR}/fps_server"
