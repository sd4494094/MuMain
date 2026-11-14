#include "stdafx.h"

#include "MagicSkeletonCloth.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/Physics/PhysicsManager.h"

#include <memory>

namespace Engine::Object::MagicSkeletonCloth
{
namespace
{
constexpr int RobeMesh = 2;
constexpr int RobeBone = 18;
constexpr float SimulationStep = 0.005f;
constexpr int SimulationIterations = 5;
constexpr float UpperCollisionRadius = 50.f;
constexpr float LowerCollisionRadius = 30.f;
constexpr float LowerCollisionOffset = -20.f;
} // namespace

void Update(CHARACTER& character)
{
    OBJECT* object = &character.Object;
    // An unavailable pose or robe cannot be simulated yet.
    if (object->BoneTransform == nullptr || !CPhysicsClothMesh::CanCreate(object->Type, RobeMesh))
        return;

    BMD* model = &Models[object->Type];
    PlaceObjectModel(object, model);
    // The update runs before any draw registers a skin request. Rebind on every
    // step: other characters share this model and can replace its cached pose.
    model->TransformCheap(object->BoneTransform, object->BoundingBoxMin, object->BoundingBoxMax, nullptr, true);

    if (object->m_pCloth == nullptr)
    {
        auto cloth = std::make_unique<CPhysicsClothMesh[]>(1);
        if (!cloth[0].Create(object, RobeMesh, RobeBone, PCT_HEAVY))
            return;
        cloth[0].AddCollisionSphere(0.f, 0.f, 0.f, UpperCollisionRadius, RobeBone);
        cloth[0].AddCollisionSphere(0.f, LowerCollisionOffset, 0.f, LowerCollisionRadius, RobeBone);
        object->m_pCloth = cloth.release();
        object->m_byNumCloth = 1;
    }
    if (!static_cast<CPhysicsCloth*>(object->m_pCloth)[0].Move2(SimulationStep, SimulationIterations))
        DeleteCloth(&character, object);
}
} // namespace Engine::Object::MagicSkeletonCloth
