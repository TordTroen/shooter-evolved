#pragma once

#include "Scene.h"

#include <Jolt/Physics/Body/MotionType.h>
#include <glm/glm.hpp>
#include <vector>

class Actor;
class Mesh;
class SpawnPoint;

class DemoScene : public Scene
{
public:
    // Pass null pointers to create a physics-only scene (server-side use).
    DemoScene(const Mesh* planeMesh, const Mesh* boxMesh);

    void setup() override;

    [[nodiscard]] const std::vector<SpawnPoint*>& spawnPoints() const { return m_spawnPoints; }

private:
    const Mesh* m_planeMesh;
    const Mesh* m_boxMesh;
    std::vector<SpawnPoint*> m_spawnPoints;

    Actor& spawnBox(glm::vec3 pos,
                    glm::vec3 scale,
                    glm::vec3 color,
                    int health,
                    JPH::EMotionType motion,
                    float mass = 0.0f);
};
