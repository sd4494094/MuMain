#include "stdafx.h"
#include "UI/NPCs/LevelingWarpDialogue.h"

#include "Core/Utilities/StringUtils.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Network/Server/WSclient.h"
#include "UI/Dialogs/GenericMenuDialog.h"

#include <algorithm>
#include <vector>

namespace
{
void SendCommand(const std::wstring& command)
{
    if (SocketClient != nullptr && Hero != nullptr)
        SocketClient->ToGameServer()->SendPublicChatMessage(MU_C16(Hero->ID), MU_C16(command.c_str()));
}

std::vector<const Network::LevelingWarpEntry*> SortedDestinations(const Network::LevelingWarpDialogPacket& dialog)
{
    std::vector<const Network::LevelingWarpEntry*> entries;
    entries.reserve(dialog.entries.size());
    for (const auto& entry : dialog.entries)
        entries.push_back(&entry);
    std::stable_sort(entries.begin(), entries.end(),
                     [](const auto* left, const auto* right) { return left->level < right->level; });
    return entries;
}

void CloseDialogue()
{
    if (SocketClient != nullptr)
        SocketClient->ToGameServer()->SendCloseNpcRequest();
}
} // namespace

namespace UI::Npc
{
void AnnounceLevelingWarpSupport()
{
    SendCommand(L"/levelwarp ready");
}

void ShowLevelingWarpDialogue(const Network::LevelingWarpDialogPacket& dialog)
{
    using namespace mu::ui::window;
    if (g_pGenericMenuDialog == nullptr || SceneFlag != MAIN_SCENE || g_pGenericMenuDialog->IsVisible())
    {
        CloseDialogue();
        return;
    }

    GenericMenuConfig config;
    config.id = "leveling-warp";
    config.title = StringUtils::NarrowToWide(dialog.title);
    for (const auto* destination : SortedDestinations(dialog))
    {
        const auto& entry = *destination;
        GenericMenuConfig::MenuButton button;
        button.label = StringUtils::NarrowToWide(entry.name);
        button.description = StringUtils::NarrowToWide(entry.description);
        button.details = StringUtils::NarrowToWide(dialog.levelLabel) + L" " + std::to_wstring(entry.level) + L"\n" +
                         StringUtils::NarrowToWide(dialog.costLabel) + L" " + std::to_wstring(entry.costs);
        button.tooltip = button.label + L" " + button.description + L"\n" + button.details;
        button.enabled = entry.enabled;
        const auto command =
            L"/levelwarp " + StringUtils::NarrowToWide(dialog.token) + L" " + std::to_wstring(entry.number);
        button.onClick = [command] { SendCommand(command); };
        config.buttons.push_back(std::move(button));
    }
    GenericMenuConfig::MenuButton close;
    close.label = StringUtils::NarrowToWide(dialog.closeLabel);
    close.compact = true;
    close.dismiss = true;
    close.onClick = CloseDialogue;
    config.buttons.push_back(std::move(close));
    config.onCancel = CloseDialogue;
    g_pGenericMenuDialog->Show(std::move(config));
}
} // namespace UI::Npc
