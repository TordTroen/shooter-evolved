#include "SpawnSelector.h"
#include "../actor/SpawnPoint.h"

#include <cassert>

RandomSpawnSelector::RandomSpawnSelector()
    : m_rng(std::random_device{}())
{
}

const SpawnPoint& RandomSpawnSelector::select(const std::vector<SpawnPoint*>& candidates)
{
    assert(!candidates.empty());
    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return *candidates[dist(m_rng)];
}
