#pragma once

#include "Core/Math/ZzzMathLib.h"

class BMD;
class CameraState;
class OBJECT;
namespace Engine::Object::Attachments
{
struct Slot;
}

namespace Render::Characters::ServiceNpcHalo
{
// Keep the entire ring above the crown and ahead of its own wings, with ordinary world depth testing.
bool Align(const OBJECT& object, const BMD& model, vec34_t* bones, const CameraState& camera, bool world,
           const Engine::Object::Attachments::Slot* wing = nullptr);
} // namespace Render::Characters::ServiceNpcHalo
