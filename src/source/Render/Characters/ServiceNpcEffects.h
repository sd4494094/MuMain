#pragma once

#include "Core/Math/ZzzMathLib.h"

class BMD;
class CHARACTER;
class OBJECT;

namespace Render::Characters::ServiceNpcEffects
{
// Called in the update only: draws consume the existing mesh look and sprites.
bool Update(CHARACTER* character, OBJECT* object, BMD* model);
void SpawnWing(BMD* model, OBJECT* object, int modelType, vec34_t* bones);
} // namespace Render::Characters::ServiceNpcEffects
