#include "stdafx.h"

#include "ServiceNpcEffects.h"
#include "ServiceNpcHalo.h"
#include "Render/Renderer/MuRenderer.h"
#include "Camera/CameraState.h"
#include "Engine/Object/ZzzCharacter.h"

#include "Data/GameData/MonsterData/ServiceNpcPresentation.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Models/ZzzBMD.h"

#include <numbers>

namespace
{
namespace Presentation = Data::Monsters::ServiceNpcPresentation;
constexpr int HaloLightCount = 8;
constexpr float HaloRadius = 19.f;
constexpr float HaloPulseRate = 0.001f;
constexpr float HaloBaseBrightness = 0.85f;
constexpr float HaloBrightnessRange = 0.1f;
constexpr float HaloSpriteScale = 0.45f;
constexpr float HaloStarScale = 0.16f;
constexpr float HaloOrbitRate = 0.00015f;
constexpr float ArmorSpriteScale = 0.22f;
constexpr float ArmorStarScale = 0.1f;
constexpr float WingSpriteScale = 0.45f;
constexpr float WingStarScale = 0.26f;
// Chest, upper arms and forearms; avoid the neck and central clavicle anchors beside the face.
constexpr int ArmorLightBones[] = {17, 26, 35, 27, 36};
constexpr int WingLightBones[] = {5, 8, 18, 19, 23, 25, 27, 38};
constexpr int WingStarBones[] = {4, 13, 26, 32};

void SpawnHaloLights(OBJECT* object, BMD* model)
{
    const float orbit = WorldTime * HaloOrbitRate;
    vec3_t gold = {0.75f, 0.67f, 0.5f};
    vec3_t white = {0.8f, 0.75f, 0.9f};
    for (int index = 0; index < HaloLightCount; ++index)
    {
        const float angle = orbit + index * (2.f * std::numbers::pi_v<float> / HaloLightCount);
        // Matches the private asset's overhead ring in halo-bone XY; the bone supplies its tilt.
        vec3_t local = {HaloRadius * std::sin(angle), HaloRadius * std::cos(angle), 0.f};
        vec3_t position;
        model->TransformPosition(object->BoneTransform[Presentation::AriaHaloBone], local, position, true);
        CreateSprite(BITMAP_LIGHT, position, HaloSpriteScale, gold, object);
        if (index % 2 == 0)
            CreateSprite(BITMAP_SHINY + 1, position, HaloStarScale, white, object, angle);
    }
}

void SpawnArmorLights(OBJECT* object, BMD* model)
{
    vec3_t local = {};
    vec3_t platinum = {0.65f, 0.6f, 0.72f};
    vec3_t violet = {0.45f, 0.32f, 0.6f};
    for (const int bone : ArmorLightBones)
    {
        vec3_t position;
        model->TransformPosition(object->BoneTransform[bone], local, position, true);
        CreateSprite(BITMAP_LIGHT, position, ArmorSpriteScale, violet, object);
        CreateSprite(BITMAP_SHINY + 1, position, ArmorStarScale, platinum, object);
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
    SpawnArmorLights(object, model);
    return true;
}

bool UpdateHaloForView(CHARACTER* character, OBJECT* object, BMD* model, const CameraState& camera)
{
    if (character->MonsterIndex != MONSTER_CREATOR_GODDESS_ARIA ||
        !ServiceNpcHalo::Align(*object, *model, object->BoneTransform, camera, false))
        return false;
    const mu::ScopedBloomExclusion classicCharacters;
    SpawnHaloLights(object, model);
    return true;
}

void UpdateHalosForView(const CameraState& camera)
{
    for (int index = 0; index < MAX_CHARACTERS_CLIENT; ++index)
    {
        CHARACTER* character = &CharactersClient[index];
        if (character->MonsterIndex != MONSTER_CREATOR_GODDESS_ARIA || !IsCharacterDrawn(character))
            continue;
        OBJECT* object = &character->Object;
        BMD* model = &Models[object->Type];
        model->BodyScale = object->Scale;
        VectorCopy(object->Position, model->BodyOrigin);
        UpdateHaloForView(character, object, model, camera);
    }
}

void SpawnWing(BMD* model, OBJECT* object, int modelType, vec34_t* bones)
{
    if (modelType != Presentation::AriaWingModel || bones == nullptr ||
        model->NumBones < Presentation::AriaWingBoneCount)
        return;
    // Preserve animated anchors, with a private palette instead of the Illusion wing's red/green lights.
    vec3_t local = {};
    vec3_t platinum = {0.65f, 0.6f, 0.72f};
    vec3_t violet = {0.45f, 0.32f, 0.6f};
    for (const int bone : WingLightBones)
    {
        vec3_t position;
        model->TransformPosition(bones[bone], local, position, true);
        CreateSprite(BITMAP_LIGHT, position, WingSpriteScale, platinum, object);
    }
    for (const int bone : WingStarBones)
    {
        vec3_t position;
        model->TransformPosition(bones[bone], local, position, true);
        CreateSprite(BITMAP_SHINY + 1, position, WingStarScale, violet, object);
    }
}
} // namespace Render::Characters::ServiceNpcEffects
