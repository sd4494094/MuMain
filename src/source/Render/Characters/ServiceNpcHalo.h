#pragma once

#include "Core/Math/ZzzMathLib.h"

class BMD;
class CameraState;
class OBJECT;

namespace Render::Characters::ServiceNpcHalo
{
// Keep the ring visually above the torso in the current perspective, preserving world height.
bool Align(const OBJECT& object, const BMD& model, vec34_t* bones, const CameraState& camera, bool world);
} // namespace Render::Characters::ServiceNpcHalo
