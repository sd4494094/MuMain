#include "stdafx.h"

#include "ServiceNpcHalo.h"

#include "Camera/CameraState.h"
#include "Data/GameData/MonsterData/ServiceNpcPresentation.h"
#include "Render/Models/ZzzBMD.h"

#include <cmath>

namespace
{
namespace Presentation = Data::Monsters::ServiceNpcPresentation;
constexpr int PelvisBone = 2;
constexpr int ChestBone = 18;
constexpr float ProjectionTolerance = 0.000001f;
constexpr float AlignmentWorldTolerance = 0.001f;

void ToWorld(const vec3_t point, const BMD& model, bool world, vec3_t result)
{
    for (int axis = 0; axis < 3; ++axis)
        result[axis] = world ? point[axis] : point[axis] * model.BodyScale + model.BodyOrigin[axis];
}

bool AlignmentDistance(const vec3_t torso, const vec3_t halo, const CameraState& camera, const vec3_t direction,
                       float& distance)
{
    vec3_t torsoEye, haloEye;
    VectorTransform(torso, camera.Matrix, torsoEye);
    VectorTransform(halo, camera.Matrix, haloEye);
    if (torsoEye[2] >= -camera.ViewNear || haloEye[2] >= -camera.ViewNear)
        return false;
    // Equal projected X means equal eye-space X/Z. Move horizontally, keeping the ring's world Z.
    const float ratio = torsoEye[0] / torsoEye[2];
    const float denominator = (camera.Matrix[0][0] * direction[0] + camera.Matrix[0][1] * direction[1]) -
                              ratio * (camera.Matrix[2][0] * direction[0] + camera.Matrix[2][1] * direction[1]);
    if (std::abs(denominator) < ProjectionTolerance)
        return false;
    distance = (ratio * haloEye[2] - haloEye[0]) / denominator;
    return std::isfinite(distance);
}
} // namespace

namespace Render::Characters::ServiceNpcHalo
{
bool Align(const OBJECT& object, const BMD& model, vec34_t* bones, const CameraState& camera, bool world)
{
    if (object.Kind != KIND_NPC || object.Type != Presentation::AriaBodyModel || bones == nullptr ||
        model.NumBones <= Presentation::AriaHaloBone || model.BodyScale <= ProjectionTolerance ||
        camera.PerspectiveX <= 0.f)
        return false;
    vec3_t torso, halo;
    for (int axis = 0; axis < 3; ++axis)
    {
        torso[axis] = (bones[PelvisBone][axis][3] + bones[ChestBone][axis][3]) * 0.5f;
        halo[axis] = bones[Presentation::AriaHaloBone][axis][3];
    }
    ToWorld(torso, model, world, torso);
    ToWorld(halo, model, world, halo);
    vec3_t direction = {camera.Matrix[0][0], camera.Matrix[0][1], 0.f};
    float distance;
    if (!AlignmentDistance(torso, halo, camera, direction, distance))
        return false;
    // Avoid sub-pixel drift when a pose is used by multiple passes in the same frame.
    if (std::abs(distance) < AlignmentWorldTolerance)
        return true;
    const float scale = world ? 1.f : model.BodyScale;
    for (int axis = 0; axis < 2; ++axis)
        bones[Presentation::AriaHaloBone][axis][3] += distance * direction[axis] / scale;
    return true;
}
} // namespace Render::Characters::ServiceNpcHalo
