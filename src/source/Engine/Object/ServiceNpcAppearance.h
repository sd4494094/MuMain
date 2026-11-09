#pragma once

#include "Core/Globals/_enum.h"

class CHARACTER;

namespace Engine::Object::ServiceNpcs
{
// Names and NPC interaction stay in the common path; appearances use their native rendering path.
CHARACTER* Create(int key, EMonsterModelType model, unsigned char x, unsigned char y);
bool HasTownHover(const CHARACTER* character);
CHARACTER* CreateLeo(int key, unsigned char x, unsigned char y);
CHARACTER* CreateAria(int key, unsigned char x, unsigned char y);
bool HasDisplayWing(const CHARACTER* character);
bool PrepareDisplayWing(CHARACTER* character);
} // namespace Engine::Object::ServiceNpcs
