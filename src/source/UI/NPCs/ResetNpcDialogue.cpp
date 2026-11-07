#include "stdafx.h"
#include "UI/NPCs/ResetNpcDialogue.h"

#include "Core/Utilities/StringUtils.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Network/Server/WSclient.h"
#include "UI/Dialogs/GenericMenuDialog.h"

#include <sstream>
#include <utility>

namespace
{
void SendResetCommand(const std::wstring& command)
{
    if (SocketClient != nullptr && Hero != nullptr)
        SocketClient->ToGameServer()->SendPublicChatMessage(MU_C16(Hero->ID), MU_C16(command.c_str()));
}

void CloseResetDialogue()
{
    if (SocketClient != nullptr)
        SocketClient->ToGameServer()->SendCloseNpcRequest();
}

mu::ui::window::GenericMenuConfig::MenuButton MakeButton(const std::string& label, std::function<void()> action)
{
    mu::ui::window::GenericMenuConfig::MenuButton button;
    button.label = StringUtils::NarrowToWide(label);
    button.onClick = std::move(action);
    return button;
}
} // namespace

namespace UI::Npc
{
void ShowResetDialogue(const std::string& token, const std::string& title, const std::string& text,
                       const std::string& queryLabel, const std::string& applyLabel, const std::string& closeLabel)
{
    using namespace mu::ui::window;
    if (g_pGenericMenuDialog == nullptr || SceneFlag != MAIN_SCENE)
        return;

    GenericMenuConfig config;
    config.id = "reset-npc";
    config.title = StringUtils::NarrowToWide(title);
    std::istringstream lines(text);
    for (std::string line; std::getline(lines, line);)
        config.lines.push_back({StringUtils::NarrowToWide(line)});
    config.buttons.push_back(MakeButton(queryLabel, [] { SendResetCommand(L"/resetinfo"); }));
    const auto command =
        token.empty() ? std::wstring(L"/reset prepare") : L"/reset confirm " + StringUtils::NarrowToWide(token);
    config.buttons.push_back(MakeButton(applyLabel, [command] { SendResetCommand(command); }));
    auto close = MakeButton(closeLabel, CloseResetDialogue);
    close.compact = true;
    close.dismiss = true;
    config.buttons.push_back(std::move(close));
    config.onCancel = CloseResetDialogue;
    g_pGenericMenuDialog->Show(std::move(config));
}
} // namespace UI::Npc
