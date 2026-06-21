#include "SpawnPoint.h"

SpawnPoint::SpawnPoint(glm::vec3 pos, float spawn_yaw)
    : yaw(spawn_yaw)
{
    position  = pos;
    maxHealth = 0; // non-damageable, non-replicated
}
