#pragma once

#include <string>
#include "GameLogic/Events/BloodCastleObjective.h"

namespace UI::Events
{
std::wstring FormatBloodCastleObjective(const GameLogic::Events::BloodCastleObjective& objective);
} // namespace UI::Events
