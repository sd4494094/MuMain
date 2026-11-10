#include "stdafx.h"
#include "UI/NPCs/AriaGuideDialogue.h"

#include "I18N/All.h"
#include "UI/Dialogs/GenericMenuDialog.h"

#include <sstream>
#include <utility>

namespace
{
void AppendSection(mu::ui::window::GenericMenuConfig& config, const wchar_t* heading, const wchar_t* text)
{
    if (heading != nullptr)
        config.lines.push_back({heading, true});
    std::wistringstream lines(text);
    for (std::wstring line; std::getline(lines, line);)
        config.lines.push_back({std::move(line)});
}

mu::ui::window::GenericMenuConfig MakeAriaGuide()
{
    using namespace mu::ui::window;
    GenericMenuConfig config;
    config.id = "aria-guide";
    config.title = I18N::Game::AriaGuideTitle;
    AppendSection(config, nullptr, I18N::Game::AriaGuideIntroduction);
    AppendSection(config, I18N::Game::AriaGuideTalkHeading, I18N::Game::AriaGuideTalk);
    AppendSection(config, I18N::Game::AriaGuideExamplesHeading, I18N::Game::AriaGuideExamples);
    AppendSection(config, I18N::Game::AriaGuideAnywhereHeading, I18N::Game::AriaGuideAnywhere);
    AppendSection(config, I18N::Game::AriaGuideAliasesHeading, I18N::Game::AriaGuideAliases);
    return config;
}
} // namespace

namespace UI::Npc
{
bool TryShowAriaGuide(const CHARACTER& speaker)
{
    using namespace mu::ui::window;
    if (SceneFlag != MAIN_SCENE || !speaker.Object.Live || speaker.Object.Kind != KIND_NPC ||
        speaker.MonsterIndex != MONSTER_CREATOR_GODDESS_ARIA || g_pGenericMenuDialog == nullptr)
        return false;
    // Aria's server greeting already releases its NPC session. Closing this local guide
    // must not send a late CloseNpcRequest which could close a different NPC's session.
    return g_pGenericMenuDialog->ShowOrUpdate(MakeAriaGuide());
}
} // namespace UI::Npc
