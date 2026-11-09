#pragma once

#include "Core/Globals/_enum.h"

namespace Data::Monsters::ServiceNpcPresentation
{
// Private asset contracts; no player item IDs or equipment models are replaced.
inline constexpr int AriaBodyModel = MODEL_MONSTER01 + static_cast<int>(MONSTER_MODEL_SERVICE_ARIA);
inline constexpr int AriaWingModel = MODEL_MONSTER01 + static_cast<int>(MONSTER_MODEL_SERVICE_ARIA_WINGS);
inline constexpr int AriaHaloMesh = 8;
inline constexpr int AriaHaloBone = 60;
inline constexpr int AriaWingBoneCount = 39;
inline constexpr float AriaWingScale = 1.08f;
inline constexpr float AriaWingOffsetZ = 15.f;
} // namespace Data::Monsters::ServiceNpcPresentation
