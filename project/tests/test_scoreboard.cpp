#include <catch2/catch_test_macros.hpp>

#include "ui/Scoreboard.h"

TEST_CASE("sort_by_kills: orders by kills descending") {
    std::vector<ScoreboardEntry> entries = {
        { NetworkId{1}, "Low",  2, 0, false },
        { NetworkId{2}, "High", 9, 0, false },
        { NetworkId{3}, "Mid",  5, 0, false },
    };

    const auto sorted = sort_by_kills(entries);

    REQUIRE(sorted.size() == 3);
    REQUIRE(sorted[0].name == "High");
    REQUIRE(sorted[1].name == "Mid");
    REQUIRE(sorted[2].name == "Low");
}

TEST_CASE("sort_by_kills: tiebreak on equal kills favors fewer deaths") {
    std::vector<ScoreboardEntry> entries = {
        { NetworkId{1}, "MoreDeaths", 3, 5, false },
        { NetworkId{2}, "FewerDeaths", 3, 1, false },
    };

    const auto sorted = sort_by_kills(entries);

    REQUIRE(sorted[0].name == "FewerDeaths");
    REQUIRE(sorted[1].name == "MoreDeaths");
}

TEST_CASE("sort_by_kills: full tiebreak chain is deterministic") {
    std::vector<ScoreboardEntry> entries = {
        { NetworkId{2}, "Bravo",   4, 2, false },
        { NetworkId{1}, "Alpha",   4, 2, false },
        { NetworkId{1}, "Zulu",    4, 2, false }, // same netId as Alpha, different name
    };

    const auto sorted = sort_by_kills(entries);

    // Equal kills+deaths → ordered by netId, then name.
    REQUIRE(sorted[0].id.value == 1);
    REQUIRE(sorted[0].name == "Alpha");
    REQUIRE(sorted[1].id.value == 1);
    REQUIRE(sorted[1].name == "Zulu");
    REQUIRE(sorted[2].id.value == 2);
}

TEST_CASE("sort_by_kills: empty input returns empty") {
    std::vector<ScoreboardEntry> entries;
    REQUIRE(sort_by_kills(entries).empty());
}

TEST_CASE("sort_by_kills: single entry is unchanged") {
    std::vector<ScoreboardEntry> entries = { { NetworkId{7}, "Solo", 3, 1, true } };

    const auto sorted = sort_by_kills(entries);

    REQUIRE(sorted.size() == 1);
    REQUIRE(sorted[0].name == "Solo");
    REQUIRE(sorted[0].kills == 3);
    REQUIRE(sorted[0].deaths == 1);
    REQUIRE(sorted[0].is_local);
}
