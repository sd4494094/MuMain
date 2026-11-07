#include "stdafx.h"

#include "ServiceNpcAppearance.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzOpenData.h"

namespace Engine::Object::ServiceNpcs
{
CHARACTER* Create(int key, EMonsterModelType model, unsigned char x, unsigned char y)
{
    OpenMonsterModel(model);
    CHARACTER* character = CreateCharacter(key, MODEL_MONSTER01 + static_cast<int>(model), x, y);
    character->NotRotateOnMagicHit = true;
    return character;
}
} // namespace Engine::Object::ServiceNpcs
