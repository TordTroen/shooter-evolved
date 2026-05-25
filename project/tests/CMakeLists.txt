add_executable(fps_demo_tests test_basic.cpp)

target_link_libraries(fps_demo_tests PRIVATE Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(fps_demo_tests)
