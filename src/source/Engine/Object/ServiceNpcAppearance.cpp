#include "stdafx.h"

#include "ServiceNpcAppearance.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzOpenData.h"

namespace
{
constexpr int ShowcaseLevel = 15;
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
    SetPlayerStop(character);
    return character;
}
} // namespace Engine::Object::ServiceNpcs
