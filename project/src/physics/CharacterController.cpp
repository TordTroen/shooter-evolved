#include "CharacterController.h"
#include "PhysicsWorld.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Collision/ShapeFilter.h>

// Capsule dimensions: half-height 0.7m, radius 0.3m → total height 2.0m.
// CharacterVirtual position is at the bottom of the capsule (feet).
// We offset the shape up by halfHeight+radius so the capsule sits above the position.
static constexpr float kCapsuleHalfHeight = 0.7f;
static constexpr float kCapsuleRadius     = 0.3f;
// Eye offset above character position (feet).
static constexpr float kEyeHeight = 1.65f;

CharacterController::CharacterController(PhysicsWorld& world, glm::vec3 startPos)
    : m_world(world)
{
    // Build a capsule shape centred at (0, halfHeight+radius, 0) in local space
    // so the character position represents the feet.
    JPH::RefConst<JPH::Shape> capsule = JPH::CapsuleShapeSettings(kCapsuleHalfHeight, kCapsuleRadius).Create().Get();
    JPH::RefConst<JPH::Shape> shape   = JPH::RotatedTranslatedShapeSettings(
        JPH::Vec3(0.0f, kCapsuleHalfHeight + kCapsuleRadius, 0.0f),
        JPH::Quat::sIdentity(),
        capsule).Create().Get();

    JPH::CharacterVirtualSettings settings;
    settings.mShape           = shape;
    settings.mMaxSlopeAngle   = JPH::DegreesToRadians(45.0f);
    settings.mUp              = JPH::Vec3::sAxisY();

    m_character = std::make_unique<JPH::CharacterVirtual>(
        &settings,
        JPH::RVec3(startPos.x, startPos.y, startPos.z),
        JPH::Quat::sIdentity(),
        &world.system());
}

CharacterController::~CharacterController() = default;

void CharacterController::update(float deltaTime, glm::vec3 wishVelocity) {
    const JPH::Vec3 gravity = m_world.system().GetGravity();
    JPH::Vec3 vel           = m_character->GetLinearVelocity();

    // Replace horizontal component with player input every frame.
    vel.SetX(wishVelocity.x);
    vel.SetZ(wishVelocity.z);

    if (m_character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround) {
        // Snap vertical velocity to ground to avoid sinking or bouncing.
        vel.SetY(std::max(0.0f, m_character->GetGroundVelocity().GetY()));
    } else {
        // Accumulate gravity while airborne.
        vel += gravity * deltaTime;
    }

    m_character->SetLinearVelocity(vel);

    JPH::DefaultBroadPhaseLayerFilter bpFilter(m_world.objectVsBroadPhaseLayerFilter(), Layers::MOVING);
    JPH::DefaultObjectLayerFilter     objFilter(m_world.objectLayerPairFilter(), Layers::MOVING);
    JPH::BodyFilter                   bodyFilter;
    JPH::ShapeFilter                  shapeFilter;

    JPH::CharacterVirtual::ExtendedUpdateSettings extSettings;
    m_character->ExtendedUpdate(deltaTime, gravity, extSettings,
                                bpFilter, objFilter, bodyFilter, shapeFilter,
                                m_world.tempAllocator());
}

glm::vec3 CharacterController::position() const {
    JPH::RVec3 p = m_character->GetPosition();
    return { static_cast<float>(p.GetX()),
             static_cast<float>(p.GetY()),
             static_cast<float>(p.GetZ()) };
}

bool CharacterController::isOnGround() const {
    return m_character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;
}

float CharacterController::eyeHeight() {
    return kEyeHeight;
}
