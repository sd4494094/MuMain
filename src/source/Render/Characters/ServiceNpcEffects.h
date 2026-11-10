#pragma once

#include "Core/Math/ZzzMathLib.h"

class BMD;
class CameraState;
class CHARACTER;
class OBJECT;

namespace Render::Characters::ServiceNpcEffects
{
// Called in the update only: draws consume the existing mesh look and sprites.
bool Update(CHARACTER* character, OBJECT* object, BMD* model);
// Halo particles depend on the final view; ordinary armor lights stay in the entity update.
bool UpdateHaloForView(CHARACTER* character, OBJECT* object, BMD* model, const CameraState& camera);
void UpdateHalosForView(const CameraState& camera);
void SpawnWing(BMD* model, OBJECT* object, int modelType, vec34_t* bones);
} // namespace Render::Characters::ServiceNpcEffects
