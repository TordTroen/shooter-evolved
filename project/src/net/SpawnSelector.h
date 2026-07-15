#pragma once

#include <random>
#include <vector>

class SpawnPoint;

class SpawnSelector
{
public:
    virtual ~SpawnSelector() = default;
    // Callers must ensure candidates is non-empty before calling.
    virtual const SpawnPoint& select(const std::vector<SpawnPoint*>& candidates) = 0;
};

// V1 implementation: uniform random selection.
// Factored behind SpawnSelector so future proximity-weighted logic is a drop-in subclass.
class RandomSpawnSelector : public SpawnSelector
{
public:
    RandomSpawnSelector();
    const SpawnPoint& select(const std::vector<SpawnPoint*>& candidates) override;

private:
    // Seeded once at construction. Spawn selection is server-only - clients receive the
    // result via snapshot - so this RNG need not be replicated (NetworkingGuidelines §5).
    std::mt19937 m_rng;
};
