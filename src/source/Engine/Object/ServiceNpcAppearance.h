#pragma once

#include "Core/Globals/_enum.h"

class CHARACTER;

namespace Engine::Object::ServiceNpcs
{
// Names and NPC interaction stay in the common path; appearances use their native rendering path.
CHARACTER* Create(int key, EMonsterModelType model, unsigned char x, unsigned char y);
CHARACTER* CreateLeo(int key, unsigned char x, unsigned char y);
} // namespace Engine::Object::ServiceNpcs
