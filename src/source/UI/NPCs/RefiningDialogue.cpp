#include "stdafx.h"
#include "UI/NPCs/RefiningDialogue.h"

#include "GameLogic/Items/MixMgr.h"
#include "I18N/All.h"
#include "Network/Server/WSclient.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Core/WindowAccess.h"
#include "UI/Dialogs/ConfirmRequest.h"

#include <utility>

namespace UI::Npc
{
void ShowRefiningDialogue()
{
    const unsigned long warningColor = RGBA(255, 0, 0, 255);
    const unsigned long descriptionColor = RGBA(223, 191, 103, 255);
    UI::Dialogs::ConfirmRequest config;
    config.tag = "osbourne-refining";
    config.lines = {
        {I18N::Game::Warning2223, true, warningColor},
        {L" ", false},
        {I18N::Game::RefineryHasStartedRefineryIsA, true, descriptionColor},
    };
    config.cancellable = true;
    config.onAccept = []
    {
        g_MixRecipeMgr.SetMixType(SEASON3A::MIXTYPE_OSBOURNE);
        UI::Windows::Show(mu::ui::window::INTERFACE_MIXINVENTORY);
    };
    config.onCancel = []
    {
        if (SocketClient != nullptr)
            SocketClient->ToGameServer()->SendCloseNpcRequest();
    };
    UI::Dialogs::ShowConfirm(std::move(config));
}
} // namespace UI::Npc
