#include "core/Paths.h"

#include <catch2/catch_test_macros.hpp>

// Full integration testing requires a known filesystem layout; that's
// covered by the manual smoke test. These tests lock in the edge-case
// return-value contract only.

TEST_CASE("Paths::setWorkingDirToProjectRoot returns false for an impossible anchor")
{
    // An anchor that cannot plausibly exist anywhere in the directory tree,
    // so the walk reaches the root without finding it → false.
    const bool result = Paths::setWorkingDirToProjectRoot("__nonexistent_anchor_xyzzy__");
    REQUIRE_FALSE(result);
}
