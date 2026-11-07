#include "ChatBubbleText.h"

#include "Core/Text/TextLineWrap.h"
#include "Core/Text/WideString.h"

#include <cwchar>

namespace UI::Chat
{
void ChatBubbleText::SetMessage(const wchar_t* message, float lifetime, bool retainPrevious)
{
    if (retainPrevious && LifeTime[0] > 0)
    {
        Core::Text::CopyWideString(Text[1], LineCapacity, Text[0]);
        LifeTime[1] = LifeTime[0];
    }
    else
    {
        Text[1][0] = L'\0';
        LifeTime[1] = 0;
    }

    if (std::wcslen(message) >= WrapLength)
    {
        CutText(message, Text[1], Text[0], LineCapacity);
        if (Text[0][0] == L'\0')
        {
            Core::Text::CopyWideString(Text[0], LineCapacity, Text[1]);
            Text[1][0] = L'\0';
        }
        LifeTime[0] = lifetime;
        LifeTime[1] = Text[1][0] == L'\0' ? 0 : lifetime;
        return;
    }

    Core::Text::CopyWideString(Text[0], LineCapacity, message);
    LifeTime[0] = lifetime;
}
} // namespace UI::Chat
