#include "TestClass.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("TestClass::test returns true")
{
    TestClass testClass;
    REQUIRE(testClass.test() == true);
}
