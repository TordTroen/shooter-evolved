#include "CharacterController.h"
#include "physics/PhysicsWorld.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Collision/ShapeFilter.h>

#include <cmath>
#include <algorithm>

// Capsule: half-height 0.7 m, radius 0.3 m → total 2.0 m.
// CharacterVirtual position = feet (bottom of capsule).
static constexpr float kCapsuleHalfHeight = 0.7f;
static constexpr float kCapsuleRadius     = 0.3f;
static constexpr float kEyeHeight         = 1.65f;
static constexpr float kMoveSpeed         = 5.0f;
static constexpr float kJumpSpeed         = 6.0f;

// Derive horizontal wish-velocity from the InputFrame's move vector and yaw.
static glm::vec3 getWishVelocity(const InputFrame& input, float speed)
{
    // Only the horizontal (yaw) component is needed for movement.
    const float yawRad   = glm::radians(input.yaw);
    const glm::vec3 flatFront = glm::normalize(glm::vec3(
        std::cos(yawRad), 0.0f, std::sin(yawRad)));
    const glm::vec3 flatRight = glm::normalize(glm::cross(
        flatFront, glm::vec3(0.0f, 1.0f, 0.0f)));

    const glm::vec3 dir = flatFront * input.move.y + flatRight * input.move.x;
    const float len = std::sqrt(dir.x * dir.x + dir.z * dir.z);
    return (len > 0.001f) ? dir * (speed / len) : glm::vec3(0.0f);
}

CharacterController::CharacterController(PhysicsWorld& world, glm::vec3 startPos)
    : m_world(world)
{
    JPH::RefConst<JPH::Shape> capsule =
        JPH::CapsuleShapeSettings(kCapsuleHalfHeight, kCapsuleRadius).Create().Get();
    JPH::RefConst<JPH::Shape> shape   =
        JPH::RotatedTranslatedShapeSettings(
            JPH::Vec3(0.0f, kCapsuleHalfHeight + kCapsuleRadius, 0.0f),
            JPH::Quat::sIdentity(), capsule).Create().Get();

    JPH::CharacterVirtualSettings settings;
    settings.mShape         = shape;
    settings.mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);
    settings.mUp            = JPH::Vec3::sAxisY();

    m_character = std::make_unique<JPH::CharacterVirtual>(
        &settings,
        JPH::RVec3(startPos.x, startPos.y, startPos.z),
        JPH::Quat::sIdentity(),
        &world.system());
}

CharacterController::~CharacterController() = default;

void CharacterController::simulate(float deltaTime, const InputFrame& input)
{
    const glm::vec3 wishVelocity = getWishVelocity(input, kMoveSpeed);

    const JPH::Vec3 gravity = m_world.system().GetGravity();
    JPH::Vec3 vel           = m_character->GetLinearVelocity();

    vel.SetX(wishVelocity.x);
    vel.SetZ(wishVelocity.z);

    if (isOnGround())
    {
        vel.SetY(std::max(0.0f, m_character->GetGroundVelocity().GetY()));
        if (input.buttons & InputFrame::kButtonJump)
            vel.SetY(kJumpSpeed);
    }
    else
    {
        vel += gravity * deltaTime;
    }

    m_character->SetLinearVelocity(vel);

    JPH::DefaultBroadPhaseLayerFilter bpFilter(
        m_world.objectVsBroadPhaseLayerFilter(), Layers::MOVING);
    JPH::DefaultObjectLayerFilter objFilter(
        m_world.objectLayerPairFilter(), Layers::MOVING);
    JPH::BodyFilter    bodyFilter;
    JPH::ShapeFilter   shapeFilter;

    JPH::CharacterVirtual::ExtendedUpdateSettings extSettings;
    m_character->ExtendedUpdate(deltaTime, gravity, extSettings,
                                bpFilter, objFilter, bodyFilter, shapeFilter,
                                m_world.tempAllocator());
}

glm::vec3 CharacterController::position() const
{
    const JPH::RVec3 p = m_character->GetPosition();
    return { static_cast<float>(p.GetX()),
             static_cast<float>(p.GetY()),
             static_cast<float>(p.GetZ()) };
}

bool CharacterController::isOnGround() const
{
    return m_character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;
}

float CharacterController::eyeHeight()
{
    return kEyeHeight;
}
