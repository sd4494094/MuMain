#pragma once

#include "Render/Models/DrawLook.h"
#include "Core/Math/ZzzMathLib.h"

class BMD;
class OBJECT;

namespace Render::Characters::ServiceNpcRendering
{
// Draw only the private Aria models; callers retain visibility, selection and shadow handling.
bool DrawBody(BMD* model, const OBJECT* object, const DrawLook& look);
// The additive halo draws after the private wing, using the same final body pose.
bool DrawHalo(BMD* model, const OBJECT* object, const DrawLook& look, vec34_t* bones, bool translate);
bool DrawWing(BMD* model, int modelType, const DrawLook& look, int level, int excellentFlags, int quality,
              int renderType);
} // namespace Render::Characters::ServiceNpcRendering
