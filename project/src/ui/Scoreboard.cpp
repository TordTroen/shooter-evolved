#include "Scoreboard.h"

#include "state/PlayerStats.h"

#include <imgui.h>

#include <algorithm>

namespace
{
    constexpr ImGuiWindowFlags kScoreboardFlags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoInputs;

    constexpr float kSummaryTopPad = 8.0f;
}

ScoreboardEntry::ScoreboardEntry(NetworkId id, std::string name, PlayerStats stats, bool is_local)
    : id(id)
    , name(name)
    , stats(stats)
    , is_local(is_local)
{
}

std::vector<ScoreboardEntry> sort_by_kills(std::vector<ScoreboardEntry> entries)
{
    std::sort(entries.begin(), entries.end(),
        [](const ScoreboardEntry& a, const ScoreboardEntry& b) {
            if (a.stats.kills != b.stats.kills) { return a.stats.kills > b.stats.kills; }
            if (a.stats.deaths != b.stats.deaths) { return a.stats.deaths < b.stats.deaths; }
            if (a.id.value != b.id.value) { return a.id.value < b.id.value; }
            return a.name < b.name;
        });
    return entries;
}

void draw_scoreboard(const std::vector<ScoreboardEntry>& sorted_entries)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        { viewport->Pos.x + viewport->Size.x * 0.5f,
          viewport->Pos.y + viewport->Size.y * 0.25f },
        ImGuiCond_Always, { 0.5f, 0.0f });
    ImGui::SetNextWindowBgAlpha(0.85f);

    if (ImGui::Begin("Scoreboard", nullptr, kScoreboardFlags))
    {
        if (ImGui::BeginTable("ScoreboardTable", 3,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Kills");
            ImGui::TableSetupColumn("Deaths");
            ImGui::TableHeadersRow();

            for (const auto& entry : sorted_entries)
            {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                if (entry.is_local)
                {
                    ImGui::TextColored({ 1.0f, 0.85f, 0.2f, 1.0f }, "%s", entry.name.c_str());
                }
                else
                {
                    ImGui::TextUnformatted(entry.name.c_str());
                }

                ImGui::TableNextColumn();
                ImGui::Text("%u", static_cast<unsigned>(entry.stats.kills));

                ImGui::TableNextColumn();
                ImGui::Text("%u", static_cast<unsigned>(entry.stats.deaths));
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void draw_score_summary(const std::vector<ScoreboardEntry>& sorted_entries, NetworkId local_id)
{
    if (sorted_entries.empty()) { return; }

    const ScoreboardEntry& leader = sorted_entries.front(); // sort_by_kills orders descending
    const auto local_it = std::find_if(sorted_entries.begin(), sorted_entries.end(),
        [local_id](const ScoreboardEntry& e) { return e.id == local_id; });
    const uint16_t local_kills = (local_it != sorted_entries.end()) ? local_it->stats.kills : 0;

    // A "tie for the lead" is any case where two or more players share the top kill
    // count — not just when the local player happens to be sort_by_kills's front()
    // entry (that slot is itself picked by the deaths/netId/name tiebreak).
    const int tied_at_top = static_cast<int>(std::count_if(sorted_entries.begin(), sorted_entries.end(),
        [top = leader.stats.kills](const ScoreboardEntry& e) { return e.stats.kills == top; }));
    const bool local_tied_for_lead = (local_kills == leader.stats.kills) && (tied_at_top > 1);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        { viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + kSummaryTopPad },
        ImGuiCond_Always, { 0.5f, 0.0f });
    ImGui::SetNextWindowBgAlpha(0.35f);

    if (ImGui::Begin("ScoreSummary", nullptr, kScoreboardFlags))
    {
        if (local_tied_for_lead)
        {
            ImGui::Text("Score: %u  (tied for lead)", static_cast<unsigned>(local_kills));
        }
        else if (leader.id == local_id)
        {
            ImGui::Text("Score: %u  (leader)", static_cast<unsigned>(local_kills));
        }
        else
        {
            ImGui::Text("Score: %u   Leader: %s (%u)", static_cast<unsigned>(local_kills),
                leader.name.c_str(), static_cast<unsigned>(leader.stats.kills));
        }
    }
    ImGui::End();
}
