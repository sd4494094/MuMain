#include "stdafx.h"
#include "doctest.h"
#include "Network/Server/WSclient.h"
#include "UI/Dialogs/GenericMenuDialog.h"
#include "UI/NPCs/LevelingWarpDialogue.h"
namespace
{
Network::LevelingWarpDialogPacket Dialog()
{
    return {"0123456789abcdef0123456789abcdef",
            "练级传送",
            "关闭",
            "等级",
            "金币",
            {{104, "天空之城", "首领·天魔菲尼斯", 170, 10000, true}, {1, "地下城一层", "幽灵房", 30, 2000, false}}};
}
void Reset()
{
    socket = {};
    SocketClient = &socket;
    Hero = &hero;
    SceneFlag = MAIN_SCENE;
    mu::ui::window::menu = {};
    mu::ui::window::g_pGenericMenuDialog = &mu::ui::window::menu;
}
} // namespace
TEST_CASE("Leveling selection captures immutable identifier and closes only on cancellation")
{
    Reset();
    auto packet = Dialog();
    UI::Npc::ShowLevelingWarpDialogue(packet);
    auto& config = mu::ui::window::menu.active;
    REQUIRE(config.buttons.size() == 3);
    CHECK(config.buttons[0].label == L"天空之城");
    CHECK(config.buttons[0].description == L"首领·天魔菲尼斯");
    CHECK_FALSE(config.buttons[1].enabled);
    packet.token = "changed";
    packet.entries[0].number = 999;
    SUBCASE("select")
    {
        config.buttons[0].onClick();
        REQUIRE(socket.connection.commands.size() == 1);
        CHECK(socket.connection.commands[0] == L"/levelwarp 0123456789abcdef0123456789abcdef 104");
        CHECK(socket.connection.closes == 0);
    }
    SUBCASE("close")
    {
        config.buttons.back().onClick();
        CHECK(socket.connection.closes == 1);
    }
    SUBCASE("escape")
    {
        config.onCancel();
        CHECK(socket.connection.closes == 1);
    }
}
TEST_CASE("Leveling menu rejects occupied UI or inappropriate scene without retaining server dialog")
{
    Reset();
    SUBCASE("occupied")
    {
        mu::ui::window::menu.visible = true;
    }
    SUBCASE("wrong scene")
    {
        SceneFlag = -1;
    }
    SUBCASE("no menu")
    {
        mu::ui::window::g_pGenericMenuDialog = nullptr;
    }
    UI::Npc::ShowLevelingWarpDialogue(Dialog());
    CHECK(socket.connection.closes == 1);
    CHECK(socket.connection.commands.empty());
}
TEST_CASE("Leveling client capability announcement and disconnection are safe")
{
    Reset();
    UI::Npc::AnnounceLevelingWarpSupport();
    REQUIRE(socket.connection.commands.size() == 1);
    CHECK(socket.connection.commands[0] == L"/levelwarp ready");
    UI::Npc::ShowLevelingWarpDialogue(Dialog());
    SocketClient = nullptr;
    auto& config = mu::ui::window::menu.active;
    CHECK_NOTHROW(config.buttons[0].onClick());
    CHECK_NOTHROW(config.onCancel());
    CHECK(socket.connection.closes == 0);
}
