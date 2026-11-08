#include "stdafx.h"
#include "doctest.h"
#include "Network/Server/WSclient.h"
#include "UI/Dialogs/GenericMenuDialog.h"
#include "UI/NPCs/ResetNpcDialogue.h"

namespace
{
void Reset()
{
    socket = {};
    SocketClient = &socket;
    Hero = &hero;
    SceneFlag = MAIN_SCENE;
    mu::ui::window::menu = {};
    mu::ui::window::g_pGenericMenuDialog = &mu::ui::window::menu;
}
void Show(const std::string& token = "", const std::string& body = "当前375级，需要400级。")
{
    UI::Npc::ShowResetDialogue(token, "转生导师·里奥", body, "查询转生条件", token.empty() ? "申请转生" : "确认转生",
                               "离开");
}
} // namespace

TEST_CASE("Reset NPC query and preparation preserve the menu and send exact commands")
{
    Reset();
    Show();
    auto& config = mu::ui::window::menu.active;
    REQUIRE(config.buttons.size() == 3);
    CHECK_FALSE(config.buttons[0].closeOnClick);
    CHECK_FALSE(config.buttons[1].closeOnClick);
    CHECK(config.buttons[2].closeOnClick);
    config.buttons[0].onClick();
    config.buttons[1].onClick();
    CHECK(socket.connection.commands == std::vector<std::wstring>{L"/resetinfo", L"/reset prepare"});
    CHECK(socket.connection.closes == 0);
    Show("", "转生需要达到400级。\n当前375级，需要400级。");
    CHECK(mu::ui::window::menu.active.lines.front().text == L"转生需要达到400级。");
    CHECK_FALSE(mu::ui::window::menu.active.buttons[1].closeOnClick);
}

TEST_CASE("Reset NPC replies replace current content and capture the latest confirmation")
{
    Reset();
    Show();
    const std::string token = "0123456789abcdef0123456789abcdef";
    Show(token, "请核对以下条件。");
    auto& config = mu::ui::window::menu.active;
    CHECK(config.buttons[1].closeOnClick);
    config.buttons[1].onClick();
    REQUIRE(socket.connection.commands.size() == 1);
    CHECK(socket.connection.commands.front() == L"/reset confirm 0123456789abcdef0123456789abcdef");
    CHECK(socket.connection.closes == 0);
}

TEST_CASE("Reset NPC cancels safely and preserves unrelated active menus")
{
    Reset();
    SUBCASE("unrelated")
    {
        mu::ui::window::menu.visible = true;
        mu::ui::window::menu.active.id = "other";
        Show();
        CHECK(mu::ui::window::menu.active.id == "other");
        CHECK(socket.connection.closes == 1);
    }
    SUBCASE("close")
    {
        Show();
        mu::ui::window::menu.active.buttons.back().onClick();
        CHECK(socket.connection.closes == 1);
    }
    SUBCASE("escape")
    {
        Show();
        mu::ui::window::menu.active.onCancel();
        CHECK(socket.connection.closes == 1);
    }
    SUBCASE("wrong scene")
    {
        SceneFlag = -1;
        Show();
        CHECK(socket.connection.closes == 1);
    }
    SUBCASE("disconnected")
    {
        Show();
        SocketClient = nullptr;
        CHECK_NOTHROW(mu::ui::window::menu.active.buttons[0].onClick());
        CHECK_NOTHROW(mu::ui::window::menu.active.onCancel());
    }
    CHECK(socket.connection.commands.empty());
}
