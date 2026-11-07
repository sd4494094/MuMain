#include "stdafx.h"
#include "UI/NPCs/StatResetDialogue.h"

#include "I18N/All.h"
#include "Network/Server/WSclient.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Dialogs/GenericMenuDialog.h"

#include <utility>

namespace
{
void CloseStatResetDialogue()
{
    if (SocketClient != nullptr)
        SocketClient->ToGameServer()->SendCloseNpcRequest();
}

void RequestStatReset()
{
    for (int i = 0; i < MAX_EQUIPMENT; ++i)
    {
        if (CharacterMachine->Equipment[i].Type != -1)
        {
            g_pSystemLogBox->AddText(I18N::Game::TheAppliedEquipmentsCannotBeReset, mu::ui::window::TYPE_ERROR_MESSAGE);
            // Generic menu actions dismiss the UI even when validation fails.
            CloseStatResetDialogue();
            return;
        }
    }
    if (SocketClient != nullptr)
        SocketClient->ToGameServer()->SendResetCharacterPointRequest();
}
} // namespace

namespace UI::Npc
{
void ShowStatResetDialogue()
{
    using namespace mu::ui::window;
    if (g_pGenericMenuDialog == nullptr)
        return;

    GenericMenuConfig config;
    config.id = "reset-points";
    config.kind = "reset-points";
    config.title = I18N::Game::ReInitializationHelper;
    config.lines.push_back({I18N::Game::ClickOnTheButtonToReinitializeAllStatPoints, false});

    GenericMenuConfig::MenuButton reset;
    reset.label = I18N::Game::StatReInitialization;
    reset.onClick = RequestStatReset;
    config.buttons.push_back(std::move(reset));

    GenericMenuConfig::MenuButton close;
    close.label = I18N::Game::Close388;
    close.compact = true;
    close.dismiss = true;
    close.onClick = CloseStatResetDialogue;
    config.buttons.push_back(std::move(close));
    config.onCancel = CloseStatResetDialogue;
    g_pGenericMenuDialog->Show(std::move(config));
}
} // namespace UI::Npc
