#include "Scene.h"

#include "actor/Actor.h"
#include "actor/components/PhysicsBody.h"

#include <algorithm>

Scene::Scene()  = default;
Scene::~Scene() = default;

void Scene::tick(float deltaTime)
{
    for (const auto& actor : m_actors)
    {
        // On clients (m_simulateReplicated = false), replicated actors are driven by
        // snapshots — skip local update so they don't fight the authoritative state.
        if (m_simulateReplicated || actor->netId == kInvalidNetworkId)
            actor->update(deltaTime);
    }

    m_physics.update(deltaTime);

    // Pull transforms back from dynamic bodies so physics drives the renderer.
    // Skipped for static/kinematic: those have an authoritative actor-side transform
    // (and the ground deliberately offsets its render quad from its physics box).
    // Also skipped for replicated actors on clients: snapshots supply their transform.
    for (const auto& actor : m_actors)
    {
        if (actor->physicsBody && actor->physicsBody->isDynamic()
            && (m_simulateReplicated || actor->netId == kInvalidNetworkId))
        {
            actor->position = actor->physicsBody->position();
            actor->rotation = actor->physicsBody->rotation();
        }
    }

    // Sweep out destroyed actors.
    // Exception: on the server (m_simulateReplicated = true) replicated actors keep
    // their slot even when destroyed so that broadcastSnapshot can send isAlive=false
    // every snapshot until the client removes them (NetworkingGuidelines §3 — persistent
    // state, self-healing). We do remove their physics body immediately so they stop
    // being simulated. On the client (m_simulateReplicated = false) they are erased
    // normally after syncFromSnapshot sets m_pendingDestroy.
    std::erase_if(m_actors, [this](const std::unique_ptr<Actor>& a)
    {
        if (!a->isPendingDestroy()) { return false; }
        if (m_simulateReplicated && a->netId != kInvalidNetworkId)
        {
            if (a->physicsBody) { a->physicsBody.reset(); } // remove from physics world
            return false; // keep the dead slot in the list
        }
        return true;
    });
}

Actor& Scene::spawn(std::unique_ptr<Actor> actor)
{
    actor->scene = this;
    // Single replication-predicate site: currently replicate anything damageable.
    // Swapping the policy later changes only should_replicate() here.
    if (actor->maxHealth > 0)
        actor->netId = assign_net_id();
    m_actors.push_back(std::move(actor));
    return *m_actors.back();
}

NetworkId Scene::assign_net_id()
{
    const NetworkId id = m_nextActorNetId;
    ++m_nextActorNetId.value;
    return id;
}

Actor* Scene::findActorByNetId(NetworkId id) const
{
    if (id == kInvalidNetworkId)
        return nullptr;

    for (const auto& actor : m_actors)
    {
        if (actor->netId == id)
            return actor.get();
    }
    return nullptr;
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
