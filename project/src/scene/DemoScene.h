#pragma once

#include "Scene.h"

#include <Jolt/Physics/Body/MotionType.h>
#include <glm/glm.hpp>
#include <functional>
#include <memory>
#include <vector>

class Actor;
class Mesh;
class MeshRenderer;
class SpawnPoint;

// Builds a MeshRenderer for a spawned actor. Injected by the caller rather than
// called directly from DemoScene.cpp so that fps_core (linked by the graphics-free
// fps_server) never references MeshRenderer's constructor symbol — MeshRenderer.cpp
// is a client-only translation unit (see HeadlessDedicatedServerPlan.md).
using MeshRendererFactory = std::function<std::unique_ptr<MeshRenderer>(const Mesh*, glm::vec3)>;

class DemoScene : public Scene
{
public:
    // Pass null mesh pointers and an empty factory to create a physics-only scene
    // (server-side use). Clients pass real meshes and a factory that constructs
    // MeshRenderer.
    DemoScene(const Mesh* planeMesh, const Mesh* boxMesh,
              MeshRendererFactory meshRendererFactory = nullptr);

    void setup() override;

    [[nodiscard]] const std::vector<SpawnPoint*>& spawnPoints() const { return m_spawnPoints; }

private:
    const Mesh* m_planeMesh;
    const Mesh* m_boxMesh;
    MeshRendererFactory m_meshRendererFactory;
    std::vector<SpawnPoint*> m_spawnPoints;

    Actor& spawnBox(glm::vec3 pos,
                    glm::vec3 scale,
                    glm::vec3 color,
                    int health,
                    JPH::EMotionType motion,
                    float mass = 0.0f);
};
