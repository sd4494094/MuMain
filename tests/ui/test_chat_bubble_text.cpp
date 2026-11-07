#include "UI/Chat/ChatBubbleText.h"

#include <doctest.h>
#include <cwchar>
#include <string>

using UI::Chat::ChatBubbleText;

TEST_CASE("NPC bubble: repeated localized idle text occupies one line [ui][npc_chat]")
{
    ChatBubbleText bubble;
    const wchar_t* line = L"我会助你踏上成为勇士的旅程。";
    for (int frame = 0; frame < 250; ++frame)
    {
        bubble.SetMessage(line, 190.f, false);
        CHECK(std::wstring(bubble.Text[0]) == line);
        CHECK(bubble.LifeTime[0] > 0);
        CHECK(bubble.Text[1][0] == L'\0');
        CHECK(bubble.LifeTime[1] == 0);
    }
}

TEST_CASE("NPC bubble: a new short line replaces old or wrapped content [ui][npc_chat]")
{
    ChatBubbleText bubble;
    bubble.SetMessage(L"I will be your strength for the journey to become a warrior.", 280.f, false);
    REQUIRE(bubble.LifeTime[0] > 0);
    REQUIRE(bubble.LifeTime[1] > 0);
    const auto first = std::wstring(bubble.Text[1]);
    const auto second = std::wstring(bubble.Text[0]);
    bubble.SetMessage(L"I will be your strength for the journey to become a warrior.", 280.f, false);
    CHECK(std::wstring(bubble.Text[1]) == first);
    CHECK(std::wstring(bubble.Text[0]) == second);
    bubble.SetMessage(L"欢迎来到勇者大陆。", 180.f, false);
    CHECK(std::wstring(bubble.Text[0]) == L"欢迎来到勇者大陆。");
    CHECK(bubble.Text[1][0] == L'\0');
    CHECK(bubble.LifeTime[1] == 0);
    bubble.SetMessage(L"请开始你的冒险。", 170.f, false);
    CHECK(std::wstring(bubble.Text[0]) == L"请开始你的冒险。");
    CHECK(bubble.LifeTime[1] == 0);
}

TEST_CASE("Player bubble: consecutive short messages still retain history [ui][npc_chat]")
{
    ChatBubbleText bubble;
    bubble.SetMessage(L"你好", 100.f, true);
    bubble.LifeTime[0] = 80.f;
    bubble.SetMessage(L"你好", 100.f, true);
    CHECK(std::wstring(bubble.Text[0]) == L"你好");
    CHECK(std::wstring(bubble.Text[1]) == L"你好");
    CHECK(bubble.LifeTime[1] == 80.f);
    bubble.SetMessage(L"继续组队", 110.f, true);
    CHECK(std::wstring(bubble.Text[0]) == L"继续组队");
    CHECK(std::wstring(bubble.Text[1]) == L"你好");
}

TEST_CASE("NPC bubble: unspaced long CJK text has one visible line [ui][npc_chat]")
{
    ChatBubbleText bubble;
    const wchar_t* line = L"欢迎来到勇者大陆请与身旁的导师交谈开始属于你的冒险之旅";
    bubble.SetMessage(line, 240.f, false);
    CHECK(std::wstring(bubble.Text[0]) == line);
    CHECK(bubble.LifeTime[0] > 0);
    CHECK(bubble.Text[1][0] == L'\0');
    CHECK(bubble.LifeTime[1] == 0);
    const std::wstring oversized(1024, L'中');
    bubble.SetMessage(oversized.c_str(), 250.f, false);
    CHECK(std::wcslen(bubble.Text[0]) == ChatBubbleText::LineCapacity - 1);
}

TEST_CASE("Chat bubble: expired history and a reused NPC slot are cleared [ui][npc_chat]")
{
    ChatBubbleText bubble;
    bubble.SetMessage(L"玩家上句", 100.f, true);
    bubble.SetMessage(L"玩家下句", 100.f, true);
    bubble.LifeTime[0] = 0;
    bubble.LifeTime[1] = 0;
    bubble.SetMessage(L"重新发言", 100.f, true);
    CHECK(bubble.LifeTime[1] == 0);
    bubble.SetMessage(L"NPC新发言", 100.f, false);
    CHECK(std::wstring(bubble.Text[0]) == L"NPC新发言");
    CHECK(bubble.LifeTime[1] == 0);
}
