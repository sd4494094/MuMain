#include "stdafx.h"
#include <doctest.h>

#include "Network/Server/WSclient.h"
#include "Data/GameData/MonsterData/MonsterDatabase.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Dialogs/GenericMenuDialog.h"
#include "UI/NPCs/StatResetDialogue.h"
#include "I18N/All.h"

namespace
{
void OpenMenu()
{
    statResetTestCharacter = {};
    statResetTestSocket = {};
    statResetTestLog = {};
    SocketClient = &statResetTestSocket;
    mu::ui::window::g_pGenericMenuDialog = &mu::ui::window::statResetTestMenu;
    UI::Npc::ShowStatResetDialogue();
}
} // namespace

TEST_CASE("closing or cancelling stat reset releases the server session without resetting")
{
    OpenMenu();
    auto& config = mu::ui::window::statResetTestMenu.active;
    REQUIRE(config.buttons.size() == 2);
    SUBCASE("close button")
    {
        REQUIRE(config.buttons[1].dismiss);
        REQUIRE(config.buttons[1].onClick);
        config.buttons[1].onClick();
    }
    SUBCASE("Escape or theme close control")
    {
        REQUIRE(config.onCancel);
        config.onCancel();
    }
    CHECK(statResetTestSocket.connection.closeRequests == 1);
    CHECK(statResetTestSocket.connection.resetRequests == 0);
}

TEST_CASE("equipment rejection closes the NPC session for every equipment slot")
{
    for (int slot = 0; slot < MAX_EQUIPMENT; ++slot)
    {
        OpenMenu();
        statResetTestCharacter.Equipment[slot].Type = 0;
        mu::ui::window::statResetTestMenu.active.buttons[0].onClick();
        CHECK(statResetTestLog.errors == 1);
        CHECK(statResetTestSocket.connection.closeRequests == 1);
        CHECK(statResetTestSocket.connection.resetRequests == 0);
    }
}

TEST_CASE("stat reset confirmation sends only the reset request")
{
    OpenMenu();
    mu::ui::window::statResetTestMenu.active.buttons[0].onClick();
    CHECK(statResetTestSocket.connection.resetRequests == 1);
    CHECK(statResetTestSocket.connection.closeRequests == 0);
    CHECK(statResetTestLog.errors == 0);
}

TEST_CASE("stat reset callbacks tolerate a disconnected socket")
{
    OpenMenu();
    SocketClient = nullptr;
    auto& config = mu::ui::window::statResetTestMenu.active;
    CHECK_NOTHROW(config.onCancel());
    CHECK_NOTHROW(config.buttons[1].onClick());
    CHECK_NOTHROW(config.buttons[0].onClick());
    CHECK(statResetTestSocket.connection.resetRequests == 0);
    CHECK(statResetTestSocket.connection.closeRequests == 0);
}

TEST_CASE("stat reset title uses the same localized name as the world NPC")
{
    g_MonsterDatabase.name = L"洗点师·艾琳";
    OpenMenu();
    CHECK(mu::ui::window::statResetTestMenu.active.title == g_MonsterDatabase.name);
    g_MonsterDatabase.name = nullptr;
    OpenMenu();
    CHECK(mu::ui::window::statResetTestMenu.active.title == I18N::Game::ReInitializationHelper);
    g_MonsterDatabase.name = L"洗点师·艾琳";
}
