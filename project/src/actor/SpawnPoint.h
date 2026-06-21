#pragma once

#include "Actor.h"
#include <glm/glm.hpp>

// A marker actor that defines a player spawn location and initial facing direction.
// No mesh, no physics body, maxHealth = 0 (non-damageable, non-replicated).
// Both server and client build the same spawn points during DemoScene::setup();
// the client's copies are inert (no netId, no mesh to render).
class SpawnPoint : public Actor
{
public:
    SpawnPoint(glm::vec3 pos, float spawn_yaw);

    float yaw = 0.0f;
};
