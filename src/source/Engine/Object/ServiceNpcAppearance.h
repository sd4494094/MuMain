#pragma once

#include "Core/Globals/_enum.h"

class CHARACTER;

namespace Engine::Object::ServiceNpcs
{
// Independent animated bodies; names and NPC interaction stay in the common path.
CHARACTER* Create(int key, EMonsterModelType model, unsigned char x, unsigned char y);
} // namespace Engine::Object::ServiceNpcs
