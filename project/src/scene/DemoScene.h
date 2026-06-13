#pragma once

#include "Scene.h"

#include <Jolt/Physics/Body/MotionType.h>
#include <glm/glm.hpp>

class Actor;
class Mesh;

class DemoScene : public Scene
{
public:
    // Pass null pointers to create a physics-only scene (server-side use).
    DemoScene(const Mesh* planeMesh, const Mesh* boxMesh);

    void setup() override;

private:
    const Mesh* m_planeMesh;
    const Mesh* m_boxMesh;

    Actor& spawnBox(glm::vec3 pos,
                    glm::vec3 scale,
                    glm::vec3 color,
                    int health,
                    JPH::EMotionType motion,
                    float mass = 0.0f);
};
