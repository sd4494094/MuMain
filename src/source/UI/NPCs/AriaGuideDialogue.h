#pragma once

class CHARACTER;

namespace UI::Npc
{
// Opens only for the live Aria NPC's server greeting; never owns a server NPC session.
bool TryShowAriaGuide(const CHARACTER& speaker);
} // namespace UI::Npc
