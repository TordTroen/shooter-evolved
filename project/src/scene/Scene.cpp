#include "Scene.h"

#include "actor/Actor.h"
#include "actor/components/PhysicsBody.h"

#include <algorithm>

Scene::Scene()  = default;
Scene::~Scene() = default;

void Scene::tick(float deltaTime)
{
    for (const auto& actor : m_actors)
        actor->update(deltaTime);

    m_physics.update(deltaTime);

    // Pull transforms back from dynamic bodies so physics drives the renderer.
    // Skipped for static/kinematic: those have an authoritative actor-side transform
    // (and the ground deliberately offsets its render quad from its physics box).
    for (const auto& actor : m_actors)
    {
        if (actor->physicsBody && actor->physicsBody->isDynamic())
        {
            actor->position = actor->physicsBody->position();
            actor->rotation = actor->physicsBody->rotation();
        }
    }

    // Sweep out destroyed actors. Their PhysicsBody destructors remove the
    // corresponding bodies from the physics world.
    std::erase_if(m_actors, [](const std::unique_ptr<Actor>& a)
    {
        return a->isPendingDestroy();
    });
}

Actor& Scene::spawn(std::unique_ptr<Actor> actor)
{
    actor->scene = this;
    m_actors.push_back(std::move(actor));
    return *m_actors.back();
}

Actor* Scene::findActorByBodyID(JPH::BodyID id) const
{
    if (id.IsInvalid())
        return nullptr;

    for (const auto& actor : m_actors)
    {
        if (actor->physicsBody && actor->physicsBody->id() == id)
            return actor.get();
    }
    return nullptr;
}
