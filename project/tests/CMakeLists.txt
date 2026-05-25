add_executable(fps_demo_tests
    test_basic.cpp
    test_test_class.cpp
    ${CMAKE_SOURCE_DIR}/src/TestClass.cpp
)

target_include_directories(fps_demo_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(fps_demo_tests PRIVATE Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(fps_demo_tests)
