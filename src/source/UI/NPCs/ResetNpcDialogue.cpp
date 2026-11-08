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

namespace
{
mu::ui::window::GenericMenuConfig MakeResetMenu(const std::string& token, const std::string& title,
                                                const std::string& text, const std::string& queryLabel,
                                                const std::string& applyLabel, const std::string& closeLabel)
{
    using namespace mu::ui::window;
    GenericMenuConfig config;
    config.id = "reset-npc";
    config.title = StringUtils::NarrowToWide(title);
    std::istringstream lines(text);
    for (std::string line; std::getline(lines, line);)
        config.lines.push_back({StringUtils::NarrowToWide(line)});
    auto query = MakeButton(queryLabel, [] { SendResetCommand(L"/resetinfo"); });
    query.closeOnClick = false;
    config.buttons.push_back(std::move(query));
    const auto command =
        token.empty() ? std::wstring(L"/reset prepare") : L"/reset confirm " + StringUtils::NarrowToWide(token);
    auto apply = MakeButton(applyLabel, [command] { SendResetCommand(command); });
    apply.closeOnClick = !token.empty();
    config.buttons.push_back(std::move(apply));
    auto close = MakeButton(closeLabel, CloseResetDialogue);
    close.compact = true;
    close.dismiss = true;
    config.buttons.push_back(std::move(close));
    config.onCancel = CloseResetDialogue;
    return config;
}
} // namespace

namespace UI::Npc
{
void ShowResetDialogue(const std::string& token, const std::string& title, const std::string& text,
                       const std::string& queryLabel, const std::string& applyLabel, const std::string& closeLabel)
{
    using namespace mu::ui::window;
    if (g_pGenericMenuDialog == nullptr || SceneFlag != MAIN_SCENE)
    {
        CloseResetDialogue();
        return;
    }
    if (!g_pGenericMenuDialog->ShowOrUpdate(MakeResetMenu(token, title, text, queryLabel, applyLabel, closeLabel)))
        CloseResetDialogue();
}
} // namespace UI::Npc
