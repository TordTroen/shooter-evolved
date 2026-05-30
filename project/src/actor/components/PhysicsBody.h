#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/EActivation.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

namespace JPH { class BodyInterface; }
class PhysicsWorld;

class PhysicsBody
{
public:
    PhysicsBody(PhysicsWorld& world,
                JPH::RefConst<JPH::Shape> shape,
                JPH::RVec3 position,
                JPH::Quat rotation,
                JPH::EMotionType motionType,
                JPH::ObjectLayer layer);
    ~PhysicsBody();

    PhysicsBody(const PhysicsBody&)            = delete;
    PhysicsBody& operator=(const PhysicsBody&) = delete;

    [[nodiscard]] JPH::BodyID id() const { return m_id; }

    void moveKinematic(JPH::RVec3 position, JPH::Quat rotation, float dt);

private:
    JPH::BodyInterface& m_bodyInterface;
    JPH::BodyID         m_id;
};
