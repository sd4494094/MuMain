#pragma once

#include <string>

namespace UI::Npc
{
// Content is UTF-8, already localized and calculated by the server.
void ShowResetDialogue(const std::string& token, const std::string& title, const std::string& text,
                       const std::string& queryLabel, const std::string& applyLabel, const std::string& closeLabel);
} // namespace UI::Npc
