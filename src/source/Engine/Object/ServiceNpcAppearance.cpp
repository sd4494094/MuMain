#include "stdafx.h"

#include "ServiceNpcAppearance.h"
#include "Data/GameData/MonsterData/ServiceNpcPresentation.h"
#include "Engine/AI/ZzzAI.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzOpenData.h"

namespace
{
constexpr int ShowcaseLevel = 15;
constexpr float HoverBoundsHeight = 70.f;
constexpr unsigned char ShowcaseExcellentFlags = 0x3F;
// These are cosmetic client parts. No item is created in a player's inventory.
void EquipShowcasePart(PART_t& part, int model)
{
    part.Type = model;
    part.Level = ShowcaseLevel;
    part.ExcellentFlags = ShowcaseExcellentFlags;
    part.AncientDiscriminator = 0;
}
} // namespace

namespace Engine::Object::ServiceNpcs
{
CHARACTER* Create(int key, EMonsterModelType model, unsigned char x, unsigned char y)
{
    OpenMonsterModel(model);
    CHARACTER* character = CreateCharacter(key, MODEL_MONSTER01 + static_cast<int>(model), x, y);
    character->NotRotateOnMagicHit = true;
    return character;
}
bool HasTownHover(const CHARACTER* character)
{
    return character->Object.Kind == KIND_NPC && character->MonsterIndex == MONSTER_LEO_THE_HELPER;
}

CHARACTER* CreateLeo(int key, unsigned char x, unsigned char y)
{
    // A merged monster mesh cannot render each item's level/excellent shaders or a linked wing.
    // Use the same player equipment path as the visual reference, retaining NPC kind in Setting_Monster.
    CHARACTER* character = CreateCharacter(key, MODEL_PLAYER, x, y);
    character->Class = CLASS_BLADEMASTER;
    character->SkinIndex = SKIN_CLASS_BLADEMASTER;
    EquipShowcasePart(character->BodyPart[BODYPART_HELM], MODEL_BLACK_DRAGON_HELM);
    EquipShowcasePart(character->BodyPart[BODYPART_ARMOR], MODEL_BLACK_DRAGON_ARMOR);
    EquipShowcasePart(character->BodyPart[BODYPART_PANTS], MODEL_BLACK_DRAGON_PANTS);
    EquipShowcasePart(character->BodyPart[BODYPART_GLOVES], MODEL_BLACK_DRAGON_GLOVES);
    EquipShowcasePart(character->BodyPart[BODYPART_BOOTS], MODEL_BLACK_DRAGON_BOOTS);
    for (auto& weapon : character->Weapon)
        EquipShowcasePart(weapon, MODEL_SWORD_OF_DESTRUCTION);
    EquipShowcasePart(character->Wing, MODEL_WING_OF_STORM);
    character->NotRotateOnMagicHit = true;
    SetCharacterScale(character);
    SetAction(&character->Object, PLAYER_STOP_FLY);
    character->Object.BoundingBoxMax[2] += HoverBoundsHeight;
    return character;
}
CHARACTER* CreateAria(int key, unsigned char x, unsigned char y)
{
    CHARACTER* character = Create(key, MONSTER_MODEL_SERVICE_ARIA, x, y);
    OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA_WINGS);
    EquipShowcasePart(character->Wing, Data::Monsters::ServiceNpcPresentation::AriaWingModel);
    character->Object.BlendMesh = Data::Monsters::ServiceNpcPresentation::AriaHaloMesh;
    return character;
}

bool HasDisplayWing(const CHARACTER* character)
{
    return character->Object.Kind == KIND_NPC && character->MonsterIndex == MONSTER_CREATOR_GODDESS_ARIA &&
           character->Object.Type == Data::Monsters::ServiceNpcPresentation::AriaBodyModel &&
           character->Wing.Type == Data::Monsters::ServiceNpcPresentation::AriaWingModel;
}

bool PrepareDisplayWing(CHARACTER* character)
{
    if (!HasDisplayWing(character))
        return false;
    constexpr unsigned char BackBone = 47;
    constexpr float FlyingWingSpeed = 0.3f;
    character->Wing.LinkBone = BackBone;
    character->Wing.PlaySpeed = FlyingWingSpeed;
    return true;
}
} // namespace Engine::Object::ServiceNpcs
