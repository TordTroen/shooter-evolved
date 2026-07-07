#pragma once

#include "net/NetworkId.h"
#include "state/PlayerStats.h"

#include <cstdint>
#include <string>
#include <vector>

struct ScoreboardEntry
{
    ScoreboardEntry(NetworkId id, std::string name, PlayerStats stats, bool is_local);
    NetworkId   id;
    std::string name;
    PlayerStats stats;
    bool        is_local = false;
};

// Pure, unit-testable. Kills descending; tiebreak deaths ascending, then netId, then name.
std::vector<ScoreboardEntry> sort_by_kills(std::vector<ScoreboardEntry> entries);

// ImGui table: Name / Kills / Deaths, local row highlighted. Call between
// ImGuiLayer::beginFrame() and endFrame().
void draw_scoreboard(const std::vector<ScoreboardEntry>& sorted_entries);

// Always-visible top-center overlay: local player's score and the leading player's
// score. sorted_entries must already be sorted by sort_by_kills (leader is entries[0]).
// No-op when entries is empty. Call between ImGuiLayer::beginFrame() and endFrame().
void draw_score_summary(const std::vector<ScoreboardEntry>& sorted_entries, NetworkId local_id);
