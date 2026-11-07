#pragma once

#include <cstddef>

namespace UI::Chat
{
// The two displayed lines, independent of the renderer and character-name lifetime.
struct ChatBubbleText
{
    static constexpr std::size_t LineCapacity = 256;
    static constexpr std::size_t WrapLength = 20;
    wchar_t Text[2][LineCapacity]{};
    float LifeTime[2]{};

    // NPC bubbles show the current utterance. Player bubbles may retain the preceding line.
    void SetMessage(const wchar_t* message, float lifetime, bool retainPrevious);
};
} // namespace UI::Chat
