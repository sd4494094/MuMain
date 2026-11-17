#include "App/stdafx.h"
#include <doctest.h>

#include "GameLogic/Events/BloodCastleObjective.h"
#include "GameLogic/Events/NewBloodCastleSystem.h"
#include "I18N/All.h"
#include "UI/Events/BloodCastleObjectiveText.h"

#include <array>
#include <string>

namespace
{
using Objective = GameLogic::Events::BloodCastleObjective;
using Phase = Objective::Phase;

PRECEIVE_MATCH_GAME_STATE State(BYTE phase, WORD required, WORD killed, BYTE item = 0xFF, WORD owner = 0xFF)
{
    PRECEIVE_MATCH_GAME_STATE result{};
    result.m_byPlayState = phase;
    result.m_wRemainSec = 769;
    result.m_wMaxKillMonster = required;
    result.m_wCurKillMonster = killed;
    result.m_byItemType = item;
    result.m_wIndex = owner;
    return result;
}

void Receive(SEASON3B::CNewBloodCastleSystem& game, PRECEIVE_MATCH_GAME_STATE packet)
{
    game.SetMatchGameCommand(&packet);
}

struct LocaleRestore
{
    std::string previous = I18N::GetCurrentLocale();
    ~LocaleRestore()
    {
        I18N::SetLocale(previous.c_str());
    }
};
} // namespace

TEST_CASE("blood castle state packets advance the objective after the last magic skeleton [events]")
{
    SEASON3B::CNewBloodCastleSystem game;
    CHECK(game.GetObjective().phase == Phase::None);

    Receive(game, State(1, 40, 23));
    CHECK(game.GetObjective().phase == Phase::KillMonsters);
    CHECK(game.GetObjective().killed == 23);
    CHECK(game.GetObjective().required == 40);

    Receive(game, State(1, 1, 0));
    CHECK(game.GetObjective().phase == Phase::DestroyGate);

    Receive(game, State(4, 2, 1));
    CHECK(game.GetObjective().phase == Phase::KillSorcerers);
    CHECK(game.GetObjective().killed == 1);
    CHECK(game.GetObjective().required == 2);

    // Actual OpenMU packet after the last kill: same state=4, both counts reset to zero.
    Receive(game, State(4, 0, 0));
    CHECK(game.GetObjective().phase == Phase::DestroyStatue);
    CHECK(game.GetMatchTime() == 769);

    LocaleRestore restore;
    I18N::SetLocale("zh-CN");
    CHECK(UI::Events::FormatBloodCastleObjective(game.GetObjective()) == L"摧毁水晶圣像");
    CHECK(UI::Events::FormatBloodCastleObjective(game.GetObjective()).find(L"0/0") == std::wstring::npos);

    // Another active packet must replace the previous target, including a new game's first phase.
    Receive(game, State(1, 40, 0));
    CHECK(game.GetObjective().phase == Phase::KillMonsters);
    CHECK(game.GetObjective().killed == 0);
}

TEST_CASE("blood castle weapon objective follows drops pickup and carrier departure [events]")
{
    SEASON3B::CNewBloodCastleSystem game;
    for (BYTE weapon : {1, 2, 3})
    {
        CAPTURE(weapon);
        // Both the current OpenMU and original protocol's no-owner markers are accepted.
        for (WORD noOwner : {0x00FF, 0xFFFF})
        {
            Receive(game, State(4, 0, 0, weapon, noOwner));
            CHECK(game.GetObjective().phase == Phase::PickUpWeapon);
        }
        // Resolve does not require the carrier to be present in the client's view.
        CHECK(Objective::FromState(4, 0, 0, weapon, 123).phase == Phase::ReturnWeapon);
        Receive(game, State(4, 0, 0, weapon, 0xFF));
        CHECK(game.GetObjective().phase == Phase::PickUpWeapon);
    }

    for (BYTE noWeapon : {0, 4, 254, 255})
        CHECK(Objective::FromState(4, 0, 0, noWeapon, 123).phase == Phase::DestroyStatue);
    CHECK(Objective::FromState(2, 0, 0, 1, 123).phase == Phase::None);
    CHECK(Objective::FromState(99, 0, 0, 1, 123).phase == Phase::None);
    CHECK(Objective::FromState(0, 40, 0, 255, 255).phase == Phase::KillMonsters);
    CHECK(Objective::FromState(1, 40, 40, 255, 255).phase == Phase::DestroyGate);
    CHECK(Objective::FromState(4, 2, 2, 255, 255).phase == Phase::DestroyStatue);
}

TEST_CASE("blood castle objectives use localized texts and retain kill progress [events][i18n]")
{
    LocaleRestore restore;
    for (const char* locale : I18N::GetAvailableLocales())
    {
        CAPTURE(locale);
        I18N::SetLocale(locale);
        for (Phase phase : {Phase::DestroyGate, Phase::DestroyStatue, Phase::PickUpWeapon, Phase::ReturnWeapon})
            CHECK_FALSE(UI::Events::FormatBloodCastleObjective({phase}).empty());
    }

    I18N::SetLocale("zh-CN");
    CHECK(UI::Events::FormatBloodCastleObjective({Phase::None}).empty());
    CHECK(UI::Events::FormatBloodCastleObjective({Phase::DestroyGate}) == L"摧毁城门");
    CHECK(UI::Events::FormatBloodCastleObjective({Phase::PickUpWeapon}) == L"拾取大天使武器");
    CHECK(UI::Events::FormatBloodCastleObjective({Phase::ReturnWeapon}) == L"归还大天使武器");
    for (Phase phase : {Phase::KillMonsters, Phase::KillSorcerers})
    {
        const auto text = UI::Events::FormatBloodCastleObjective({phase, 7, 20});
        CHECK(text.find(L"7") != std::wstring::npos);
        CHECK(text.find(L"20") != std::wstring::npos);
    }

    I18N::SetLocale("en");
    CHECK(UI::Events::FormatBloodCastleObjective({Phase::DestroyStatue}) == L"Destroy the crystal statue");
}
