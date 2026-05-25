@echo off
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON || exit /b 1
cmake --build cmake-build-debug --target fps_demo_tests || exit /b 1
ctest --test-dir cmake-build-debug --output-on-failure