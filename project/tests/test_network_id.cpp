#include <catch2/catch_test_macros.hpp>

#include "Net/NetworkId.h"

TEST_CASE("NetworkId")
{
	SECTION("Invalid")
	{
		NetworkId UnInitializedNetId;
		CHECK(UnInitializedNetId == kInvalidNetworkId);
		CHECK(!UnInitializedNetId);
		if (UnInitializedNetId)
		{
			FAIL("Uninitialized NetworkId returns true");
		}
	}

	SECTION("Equality")
	{
		NetworkId A(100);
		CHECK(A == A);
		NetworkId B(100);
		CHECK(A == B);
	}

	SECTION("Inequality")
	{
		NetworkId A(100);
		NetworkId B(200);
		CHECK(A != B);
	}
}