#include "stdafx.h"

#include "ServiceNpcHalo.h"

#include "Camera/CameraState.h"
#include "Data/GameData/MonsterData/ServiceNpcPresentation.h"
#include "Engine/Object/CharacterAttachments.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Models/ZzzBMD.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
namespace Presentation = Data::Monsters::ServiceNpcPresentation;
namespace Attachments = Engine::Object::Attachments;
constexpr int PelvisBone = 2;
constexpr int ChestBone = 18;
constexpr int HeadBone = 20;
constexpr int FaceMesh = 0;
// Bounds of the private double ring, including its tube thickness; verified against the actual vertices.
constexpr vec3_t HaloHalfExtent = {24.f, 24.f, 1.2f};
constexpr float HeadGap = 8.f;
constexpr float WingDepthGap = 2.f;
constexpr float PositionTolerance = 0.001f;
constexpr float ScaleTolerance = 0.000001f;

void ToEye(const vec3_t point, const BMD& model, bool world, const CameraState& camera, vec3_t eye)
{
    vec3_t position;
    for (int axis = 0; axis < 3; ++axis)
        position[axis] = world ? point[axis] : point[axis] * model.BodyScale + model.BodyOrigin[axis];
    VectorTransform(position, camera.Matrix, eye);
}

bool HeadBounds(const BMD& model, const vec34_t* bones, bool world, const CameraState& camera, float& top, float& front)
{
    vec3_t point, eye;
    for (int axis = 0; axis < 3; ++axis)
        point[axis] = bones[HeadBone][axis][3];
    ToEye(point, model, world, camera, eye);
    if (eye[2] >= -camera.ViewNear)
        return false;
    top = eye[1] / -eye[2];
    front = eye[2];
    if (model.Meshs == nullptr || model.NumMeshs <= FaceMesh)
        return true;
    const auto& mesh = model.Meshs[FaceMesh];
    for (int index = 0; index < mesh.NumVertices; ++index)
    {
        const auto& vertex = mesh.Vertices[index];
        VectorTransform(vertex.Position, bones[vertex.Node], point);
        ToEye(point, model, world, camera, eye);
        if (eye[2] >= -camera.ViewNear)
            return false;
        top = std::max(top, eye[1] / -eye[2]);
        front = std::max(front, eye[2]);
    }
    return true;
}

const Attachments::Slot* DisplayWing(const OBJECT& object)
{
    if (CharactersClient == nullptr)
        return nullptr;
    for (int index = 0; index < MAX_CHARACTERS_CLIENT; ++index)
    {
        const CHARACTER* character = &CharactersClient[index];
        if (&character->Object == &object)
            return Attachments::Find(character, &character->Wing, Presentation::AriaWingModel, true);
    }
    return nullptr;
}

float WingFrontDepth(const Attachments::Slot* wing, const CameraState& camera, float front)
{
    if (wing == nullptr || wing->type != Presentation::AriaWingModel || wing->bones == nullptr)
        return front;
    const BMD& model = Models[wing->type];
    for (int meshIndex = 0; meshIndex < model.NumMeshs; ++meshIndex)
    {
        const auto& mesh = model.Meshs[meshIndex];
        for (int index = 0; index < mesh.NumVertices; ++index)
        {
            const auto& vertex = mesh.Vertices[index];
            vec3_t point, eye;
            VectorTransform(vertex.Position, wing->bones[vertex.Node], point);
            for (int axis = 0; axis < 3; ++axis)
                point[axis] = point[axis] * wing->posed.scale + wing->posed.origin[axis];
            VectorTransform(point, camera.Matrix, eye);
            front = std::max(front, eye[2]);
        }
    }
    return front;
}

void RingCorners(const vec34_t bone, float scale, const CameraState& camera, vec3_t* corners)
{
    for (int corner = 0; corner < 8; ++corner)
    {
        vec3_t local, rotated;
        for (int axis = 0; axis < 3; ++axis)
            local[axis] = (corner & (1 << axis) ? 1.f : -1.f) * HaloHalfExtent[axis];
        VectorRotate(local, bone, rotated);
        VectorScale(rotated, scale, rotated);
        VectorRotate(rotated, camera.Matrix, corners[corner]);
    }
}

bool PlaceRing(const vec3_t torso, float headTop, float headFront, float wingFront, const vec3_t* corners, float scale,
               const CameraState& camera, vec3_t world)
{
    float nearOffset = std::numeric_limits<float>::max();
    for (int index = 0; index < 8; ++index)
        nearOffset = std::min(nearOffset, corners[index][2]);
    vec3_t center;
    center[2] = wingFront - nearOffset + WingDepthGap * scale;
    if (center[2] >= -camera.ViewNear)
        return false;
    center[0] = torso[0] / torso[2] * center[2];
    const float aboveCrown = headTop + HeadGap * scale / -headFront;
    center[1] = std::numeric_limits<float>::lowest();
    for (int index = 0; index < 8; ++index)
        center[1] = std::max(center[1], aboveCrown * (-center[2] - corners[index][2]) - corners[index][1]);
    vec3_t translated;
    for (int axis = 0; axis < 3; ++axis)
        translated[axis] = center[axis] - camera.Matrix[axis][3];
    VectorIRotate(translated, camera.Matrix, world);
    return std::isfinite(world[0]) && std::isfinite(world[1]) && std::isfinite(world[2]);
}
} // namespace

namespace Render::Characters::ServiceNpcHalo
{
bool Align(const OBJECT& object, const BMD& model, vec34_t* bones, const CameraState& camera, bool world,
           const Attachments::Slot* wing)
{
    if (object.Kind != KIND_NPC || object.Type != Presentation::AriaBodyModel || bones == nullptr ||
        model.NumBones <= Presentation::AriaHaloBone || model.BodyScale <= ScaleTolerance || camera.PerspectiveX <= 0.f)
        return false;
    vec3_t torso, eye;
    for (int axis = 0; axis < 3; ++axis)
        torso[axis] = (bones[PelvisBone][axis][3] + bones[ChestBone][axis][3]) * 0.5f;
    ToEye(torso, model, world, camera, eye);
    float headTop, headFront;
    if (eye[2] >= -camera.ViewNear || !HeadBounds(model, bones, world, camera, headTop, headFront))
        return false;
    const float wingFront = WingFrontDepth(wing != nullptr ? wing : DisplayWing(object), camera, headFront);
    vec3_t corners[8], target;
    RingCorners(bones[Presentation::AriaHaloBone], world ? 1.f : model.BodyScale, camera, corners);
    if (!PlaceRing(eye, headTop, headFront, wingFront, corners, model.BodyScale, camera, target))
        return false;
    for (int axis = 0; axis < 3; ++axis)
    {
        const float position = world ? target[axis] : (target[axis] - model.BodyOrigin[axis]) / model.BodyScale;
        if (std::abs(position - bones[Presentation::AriaHaloBone][axis][3]) > PositionTolerance)
            bones[Presentation::AriaHaloBone][axis][3] = position;
    }
    return true;
}
} // namespace Render::Characters::ServiceNpcHalo
