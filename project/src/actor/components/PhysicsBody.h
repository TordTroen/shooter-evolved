#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/EActivation.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace JPH { class BodyInterface; }
class PhysicsWorld;

class PhysicsBody
{
public:
    // mass <= 0 lets Jolt derive mass from the shape (volume × default density).
    // Only honoured for Dynamic bodies; static/kinematic bodies have infinite mass.
    PhysicsBody(PhysicsWorld& world,
                JPH::RefConst<JPH::Shape> shape,
                JPH::RVec3 position,
                JPH::Quat rotation,
                JPH::EMotionType motionType,
                JPH::ObjectLayer layer,
                float mass = 0.0f);
    ~PhysicsBody();

    PhysicsBody(const PhysicsBody&)            = delete;
    PhysicsBody& operator=(const PhysicsBody&) = delete;

    [[nodiscard]] JPH::BodyID id() const { return m_id; }
    [[nodiscard]] bool isDynamic()  const { return m_motionType == JPH::EMotionType::Dynamic; }
    [[nodiscard]] bool isKinematic() const { return m_motionType == JPH::EMotionType::Kinematic; }
    [[nodiscard]] bool isStatic()   const { return m_motionType == JPH::EMotionType::Static; }

    void moveKinematic(JPH::RVec3 position, JPH::Quat rotation, float dt);
    void moveKinematic(glm::vec3 position, glm::quat rotation, float dt); // glm convenience overload

    // Convert the body to the given motion type in-place (used on clients to make
    // replicated bodies kinematic snapshot-slaved proxies — NetworkingGuidelines §1/D1).
    void set_motion_type(JPH::EMotionType type);

    // No-op on non-dynamic bodies.
    void applyImpulse(glm::vec3 impulse, glm::vec3 worldPos);

    [[nodiscard]] glm::vec3 position() const;
    [[nodiscard]] glm::quat rotation() const;

private:
    JPH::BodyInterface& m_bodyInterface;
    JPH::BodyID         m_id;
    JPH::EMotionType    m_motionType;
};
