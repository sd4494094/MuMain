#include "stdafx.h"

#include "ServiceNpcEffects.h"

#include "Data/GameData/MonsterData/ServiceNpcPresentation.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Items/ItemEffects.h"
#include "Render/Models/ZzzBMD.h"

#include <numbers>

namespace
{
namespace Presentation = Data::Monsters::ServiceNpcPresentation;
constexpr int HaloLightCount = 8;
constexpr float HaloRadius = 30.f;
constexpr float HaloPulseRate = 0.002f;
constexpr float HaloBaseBrightness = 1.f;
constexpr float HaloBrightnessRange = 0.2f;
constexpr float HaloSpriteScale = 0.55f;
constexpr float HaloStarScale = 0.18f;
constexpr float HaloOrbitRate = 0.00025f;

void SpawnHaloLights(OBJECT* object, BMD* model)
{
    const float orbit = WorldTime * HaloOrbitRate;
    vec3_t gold = {1.f, 0.78f, 0.32f};
    vec3_t white = {1.f, 0.94f, 0.74f};
    for (int index = 0; index < HaloLightCount; ++index)
    {
        const float angle = orbit + index * (2.f * std::numbers::pi_v<float> / HaloLightCount);
        vec3_t local = {HaloRadius * std::sin(angle), 0.f, HaloRadius * std::cos(angle)};
        vec3_t position;
        model->TransformPosition(object->BoneTransform[Presentation::AriaHaloBone], local, position, true);
        CreateSprite(BITMAP_LIGHT, position, HaloSpriteScale, gold, object);
        if (index % 2 == 0)
            CreateSprite(BITMAP_SHINY + 1, position, HaloStarScale, white, object, angle);
    }
}
} // namespace

namespace Render::Characters::ServiceNpcEffects
{
bool Update(CHARACTER* character, OBJECT* object, BMD* model)
{
    if (character->MonsterIndex != MONSTER_CREATOR_GODDESS_ARIA || object->Kind != KIND_NPC ||
        object->Type != Presentation::AriaBodyModel)
        return false;
    if (object->BoneTransform == nullptr || model->NumBones <= Presentation::AriaHaloBone ||
        model->NumMeshs <= Presentation::AriaHaloMesh)
        return true;
    object->BlendMesh = Presentation::AriaHaloMesh;
    object->BlendMeshLight = HaloBaseBrightness + HaloBrightnessRange * std::sin(WorldTime * HaloPulseRate);
    SpawnHaloLights(object, model);
    return true;
}

void SpawnWing(BMD* model, OBJECT* object, int modelType, vec34_t* bones)
{
    if (modelType != Presentation::AriaWingModel || bones == nullptr ||
        model->NumBones < Presentation::AriaWingBoneCount)
        return;
    // The private model retains the Illusion wing's complete, unchanged bone layout.
    // Its own model identity keeps +15/excellent glow independent of ordinary wing policy.
    Items::ItemEffects::Spawn(model, object, MODEL_WING_OF_ILLUSION, bones);
}
} // namespace Render::Characters::ServiceNpcEffects
