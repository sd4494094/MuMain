#include "stdafx.h"

#include "BloodCastleObjectiveText.h"
#include "I18N/All.h"

namespace UI::Events
{
std::wstring FormatBloodCastleObjective(const GameLogic::Events::BloodCastleObjective& objective)
{
    using Phase = GameLogic::Events::BloodCastleObjective::Phase;
    const wchar_t* format = nullptr;
    switch (objective.phase)
    {
    case Phase::None:
        return {};
    case Phase::DestroyGate:
        return I18N::Game::BloodCastleDestroyGate;
    case Phase::DestroyStatue:
        return I18N::Game::BloodCastleDestroyStatue;
    case Phase::PickUpWeapon:
        return I18N::Game::BloodCastlePickUpWeapon;
    case Phase::ReturnWeapon:
        return I18N::Game::BloodCastleReturnWeapon;
    case Phase::KillMonsters:
        format = I18N::Game::MonsterDD;
        break;
    case Phase::KillSorcerers:
        format = I18N::Game::MagicSkeletonDD;
        break;
    }

    constexpr std::size_t kObjectiveTextCapacity = 256;
    wchar_t text[kObjectiveTextCapacity]{};
    if (format != nullptr)
        mu_swprintf(text, format, objective.killed, objective.required);
    return text;
}
} // namespace UI::Events
