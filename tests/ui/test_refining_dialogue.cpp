#include "stdafx.h"
#include <doctest.h>

#include "GameLogic/Items/MixMgr.h"
#include "Network/Server/WSclient.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Dialogs/ConfirmRequest.h"
#include "UI/NPCs/RefiningDialogue.h"

namespace UI::Dialogs
{
ConfirmRequest refiningTestRequest;
void ShowConfirm(ConfirmRequest request)
{
    refiningTestRequest = std::move(request);
}
} // namespace UI::Dialogs

TEST_CASE("refining cancellation releases the NPC without opening the crafting window")
{
    statResetTestSocket = {};
    SocketClient = &statResetTestSocket;
    g_MixRecipeMgr = {};
    refiningTestWindow = -1;
    UI::Npc::ShowRefiningDialogue();
    auto& request = UI::Dialogs::refiningTestRequest;
    REQUIRE(request.cancellable);
    REQUIRE(request.onCancel);
    request.onCancel();
    CHECK(statResetTestSocket.connection.closeRequests == 1);
    CHECK(g_MixRecipeMgr.type == -1);
    CHECK(refiningTestWindow == -1);
}

TEST_CASE("accepting refining retains the NPC session for the crafting window")
{
    statResetTestSocket = {};
    SocketClient = &statResetTestSocket;
    UI::Npc::ShowRefiningDialogue();
    REQUIRE(UI::Dialogs::refiningTestRequest.onAccept);
    UI::Dialogs::refiningTestRequest.onAccept();
    CHECK(g_MixRecipeMgr.type == SEASON3A::MIXTYPE_OSBOURNE);
    CHECK(refiningTestWindow == mu::ui::window::INTERFACE_MIXINVENTORY);
    CHECK(statResetTestSocket.connection.closeRequests == 0);
}

TEST_CASE("refining cancellation tolerates disconnect")
{
    UI::Npc::ShowRefiningDialogue();
    SocketClient = nullptr;
    CHECK_NOTHROW(UI::Dialogs::refiningTestRequest.onCancel());
}
