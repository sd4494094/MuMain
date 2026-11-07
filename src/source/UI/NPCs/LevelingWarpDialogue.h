#pragma once

#include "Network/Server/LevelingWarpDialogPacket.h"

namespace UI::Npc
{
void ShowLevelingWarpDialogue(const Network::LevelingWarpDialogPacket& dialog);
void AnnounceLevelingWarpSupport();
} // namespace UI::Npc
