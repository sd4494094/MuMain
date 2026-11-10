#include "stdafx.h"
#include <doctest.h>
#include "RmlLayoutFixture.h"
#include "UI/Core/WindowManager.h"
#include "UI/Dialogs/GenericMenuDialog.h"
#include "UI/NPCs/AriaGuideDialogue.h"
#include "I18N/All.h"
#include "UI/RmlBridge/RmlClickSounds.h"
#include "UI/RmlBridge/RmlElementBox.h"
#include "UI/RmlBridge/RmlTheme.h"
#include <filesystem>
#include "UI/RmlBridge/RmlNativeTextSize.h"

namespace
{
using mu::ui::window::GenericMenuConfig;

class MenuFixture
{
public:
    MenuFixture(const std::string& theme, Rml::Vector2i viewport, int scale)
        : previousDirectory(std::filesystem::current_path()), previousTheme(UI::RmlBridge::GetActiveThemeName())
    {
        UI::RmlBridge::RegisterClickSoundProperty();
        REQUIRE(Rml::LoadFontFace(std::string(MU_FONT_DIR) + "/NotoSansSC-Regular.otf", true));
        previousLocale = I18N::GetCurrentLocale();
        I18N::SetLocale("zh-CN");
        previousScene = SceneFlag;
        SceneFlag = MAIN_SCENE;
        previousMenu = mu::ui::window::g_pGenericMenuDialog;
        mu::ui::window::g_pGenericMenuDialog = &dialog;
        previousWidth = WindowWidth;
        previousHeight = WindowHeight;
        WindowWidth = viewport.x;
        WindowHeight = viewport.y;
        layout.Configure(viewport, scale);
        std::filesystem::current_path(std::filesystem::path(MU_RMLUI_DIR).parent_path().parent_path().parent_path());
        UI::RmlBridge::SetActiveThemeName(theme);
        UI::RmlBridge::SetContextResolver([this](std::string_view) { return layout.context; });
        dialog.Create(&manager);
        document = layout.context->GetDocument(0);
        REQUIRE(document);
    }
    ~MenuFixture()
    {
        I18N::SetLocale(previousLocale.c_str());
        SceneFlag = previousScene;
        mu::ui::window::g_pGenericMenuDialog = previousMenu;
        WindowWidth = previousWidth;
        WindowHeight = previousHeight;
        dialog.Release();
        manager.RemoveUIObj(&dialog);
        UI::RmlBridge::SetContextResolver({});
        std::filesystem::current_path(previousDirectory);
        UI::RmlBridge::SetActiveThemeName(previousTheme);
    }
    void SelectDocument()
    {
        document = dialog.GetPlacedDocument();
        if (!document)
            document = layout.context->GetDocument(0);
        const auto dock =
            UI::Scaling::DockRightTransform(layout.context->GetDimensions().x, layout.context->GetDimensions().y);
        if (dialog.IsNpcMenuVisible())
        {
            document->UpdateDocument();
            const float width = document->GetElementById("panel")->GetBox().GetSize(Rml::BoxArea::Border).x;
            dialog.PlaceDocument(layout.context->GetDimensions().x - width * dock.scaleX, std::max(0.f, dock.offsetY),
                                 dock.scaleX);
            CHECK_FALSE(document->GetElementById("panel")->IsClassSet("center-both"));
        }
        layout.Refresh();
    }
    void CheckFixedPanel(const std::string& expectedTitle = "转生导师·里奥")
    {
        SelectDocument();
        auto* panel = document->GetElementById("panel");
        auto* title = document->GetElementById("title");
        REQUIRE(panel);
        REQUIRE(title);
        CHECK(title->GetInnerRML() == expectedTitle);
        CHECK(title->GetComputedValues().font_size() ==
              doctest::Approx(UI::RmlBridge::NativeTextPx(UI::Scaling::FontRole::Normal)));
        Rml::Vector2f before, size;
        REQUIRE(UI::RmlBridge::DrawnBox(*panel, Rml::BoxArea::Border, before, size));
        const auto start = before + Rml::Vector2f(size.x * .5f, size.y * .1f);
        layout.context->ProcessMouseMove(static_cast<int>(start.x), static_cast<int>(start.y), 0);
        layout.context->ProcessMouseButtonDown(0, 0);
        layout.context->ProcessMouseMove(static_cast<int>(start.x - 100), static_cast<int>(start.y + 50), 0);
        layout.context->ProcessMouseButtonUp(0, 0);
        layout.Refresh();
        Rml::Vector2f after, afterSize;
        REQUIRE(UI::RmlBridge::DrawnBox(*panel, Rml::BoxArea::Border, after, afterSize));
        CHECK(after.x == doctest::Approx(before.x));
        CHECK(after.y == doctest::Approx(before.y));
        CHECK_FALSE(panel->IsClassSet("dragged"));
    }
    void Click(int index)
    {
        SelectDocument();
        Rml::ElementList buttons;
        Rml::ElementList allButtons;
        document->QuerySelectorAll(allButtons, ".gmd-btn");
        for (auto* button : allButtons)
            if (button->IsVisible(true))
                buttons.push_back(button);
        REQUIRE(index >= 0);
        REQUIRE(index < static_cast<int>(buttons.size()));
        Rml::Vector2f position, size;
        REQUIRE(UI::RmlBridge::DrawnBox(*buttons[index], Rml::BoxArea::Border, position, size));
        const auto point = position + size * .5f;
        layout.context->ProcessMouseMove(static_cast<int>(point.x), static_cast<int>(point.y), 0);
        layout.context->ProcessMouseButtonDown(0, 0);
        layout.context->ProcessMouseButtonUp(0, 0);
        layout.Refresh();
        dialog.Update();
        layout.Refresh();
    }
    void CheckResetContentFits()
    {
        SelectDocument();
        auto* body = document->GetElementById("npc_lines_container");
        auto* answers = document->GetElementById("answers_container");
        auto* close = document->GetElementById("btn_exit");
        REQUIRE(body);
        REQUIRE(answers);
        REQUIRE(close);
        CHECK(body->GetScrollHeight() <= body->GetClientHeight() + 1.f);
        CHECK(answers->GetScrollHeight() <= answers->GetClientHeight() + 1.f);
        CHECK(body->GetComputedValues().overflow_y() == Rml::Style::Overflow::Hidden);
        CHECK(answers->GetComputedValues().overflow_y() == Rml::Style::Overflow::Hidden);
        Rml::Vector2f bodyPosition, bodySize, answersPosition, answersSize, closePosition, closeSize;
        REQUIRE(UI::RmlBridge::DrawnBox(*body, Rml::BoxArea::Border, bodyPosition, bodySize));
        REQUIRE(UI::RmlBridge::DrawnBox(*answers, Rml::BoxArea::Border, answersPosition, answersSize));
        REQUIRE(UI::RmlBridge::DrawnBox(*close, Rml::BoxArea::Border, closePosition, closeSize));
        CHECK(bodyPosition.y + bodySize.y <= answersPosition.y);
        CHECK(answersPosition.y + answersSize.y <= closePosition.y + 1.f);
        Rml::ElementList lines;
        body->QuerySelectorAll(lines, ".gmd-line");
        for (auto* line : lines)
        {
            if (!line->IsVisible(true))
                continue;
            Rml::Vector2f position, size;
            REQUIRE(UI::RmlBridge::DrawnBox(*line, Rml::BoxArea::Border, position, size));
            CHECK(position.y >= bodyPosition.y - 1.f);
            CHECK(position.y + size.y <= bodyPosition.y + bodySize.y + 1.f);
        }
    }
    void CheckFirstLine(const std::string& text)
    {
        SelectDocument();
        auto* body = document->QuerySelector(".gmd-lines");
        REQUIRE(body);
        Rml::ElementList lines;
        body->QuerySelectorAll(lines, ".gmd-line");
        auto found = std::find_if(lines.begin(), lines.end(), [](auto* line) { return line->IsVisible(true); });
        auto* line = found == lines.end() ? nullptr : *found;
        REQUIRE(line);
        CHECK(line->GetInnerRML().find(text) != Rml::String::npos);
        Rml::Vector2f bodyPosition, bodySize, linePosition, lineSize;
        REQUIRE(UI::RmlBridge::DrawnBox(*body, Rml::BoxArea::Border, bodyPosition, bodySize));
        REQUIRE(UI::RmlBridge::DrawnBox(*line, Rml::BoxArea::Border, linePosition, lineSize));
        CHECK(lineSize.y > 0.f);
        CHECK(linePosition.y >= bodyPosition.y - 1.f);
        CHECK(linePosition.y + lineSize.y <= bodyPosition.y + bodySize.y + 1.f);
        CHECK(body->GetScrollTop() == doctest::Approx(0.f));
    }

    EGameScene previousScene = MAIN_SCENE;
    std::string previousLocale;
    mu::ui::window::CGenericMenuDialog* previousMenu = nullptr;
    UI::Tests::RmlLayoutFixture layout;
    std::filesystem::path previousDirectory;
    std::string previousTheme;
    mu::ui::window::CManager manager;
    mu::ui::window::CGenericMenuDialog dialog;
    Rml::ElementDocument* document = nullptr;
    unsigned int previousWidth = 0, previousHeight = 0;
};

GenericMenuConfig Menu(int& selected)
{
    GenericMenuConfig config;
    config.id = "reset-npc";
    config.title = L"转生导师·里奥";
    config.lines = {{L"下一次：第1次转生；当前375级，需要400级。"},
                    {L"需要金币：2；当前持有：81001614。"},
                    {L"需要物品：创造宝石×10；当前持有：10。"},
                    {L"转生次数上限：1000。"},
                    {L"转生后10级；本次奖励1500点，可分配点数合计1500。"},
                    {L"已分配属性恢复为职业初始值。"},
                    {L"成功后回到出生点。"},
                    {L"成功后返回角色选择。"},
                    {L"查询和申请不会扣费；只有点击确认转生才会执行。"}};
    for (int i = 0; i < 3; ++i)
    {
        GenericMenuConfig::MenuButton button;
        button.label = i == 0 ? L"查询转生条件" : i == 1 ? L"申请转生" : L"离开";
        button.closeOnClick = i == 2;
        button.compact = button.dismiss = i == 2;
        button.onClick = [&, i] { selected = i; };
        config.buttons.push_back(std::move(button));
    }
    config.onCancel = [&] { selected = -2; };
    return config;
}

void CheckReplySequence(MenuFixture& fixture)
{
    int selected = -1;
    auto config = Menu(selected);
    REQUIRE(fixture.dialog.ShowOrUpdate(config));
    REQUIRE(fixture.dialog.ShowOrUpdate(config)); // a repeated opening must not create stale pages
    fixture.SelectDocument();
    fixture.CheckFixedPanel();
    fixture.CheckResetContentFits();
    for (int index : {0, 1, 0, 1})
    {
        CAPTURE(index);
        fixture.Click(index);
        CHECK(selected == index);
        CHECK(fixture.dialog.IsVisible());
        CHECK(fixture.document->IsVisible()); // including the round trip before a response exists
        config.lines.insert(config.lines.begin(),
                            {index == 0 ? L"已刷新当前转生条件，尚未执行转生。" : L"转生需要达到400级。"});
        REQUIRE(fixture.dialog.ShowOrUpdate(config));
        fixture.CheckFirstLine(index == 0 ? "已刷新当前转生条件" : "转生需要达到400级");
        fixture.CheckResetContentFits();
        config.lines.erase(config.lines.begin());
    }
    fixture.Click(2);
    CHECK(selected == 2);
    CHECK_FALSE(fixture.dialog.IsVisible());
    CHECK_FALSE(fixture.document->IsVisible()); // no queued old replies reopen after leaving
    config.id = "other";
    fixture.dialog.Show(config);
    fixture.layout.Refresh();
    fixture.CheckFirstLine("下一次");
}
} // namespace

TEST_CASE("Reset menu updates replies in place without flicker or stale pages [ui][reset-menu]")
{
    for (const std::string theme : {"legacy", "classic", "modern"})
        for (const Rml::Vector2i viewport : {Rml::Vector2i{800, 600}, {1280, 720}, {1920, 1080}, {3440, 1440}})
            for (int scale : {75, 100, 125, 150})
            {
                CAPTURE(theme);
                CAPTURE(viewport.x);
                CAPTURE(viewport.y);
                CAPTURE(scale);
                MenuFixture fixture(theme, viewport, scale);
                CheckReplySequence(fixture);
            }
}

TEST_CASE("Generic menu preserves unrelated pages and synchronous callback ownership [ui][reset-menu]")
{
    MenuFixture fixture("legacy", {1920, 1080}, 100);
    int selected = -1;
    auto other = Menu(selected);
    other.id = "other";
    fixture.dialog.Show(other);
    CHECK_FALSE(fixture.dialog.ShowOrUpdate(Menu(selected)));
    fixture.layout.Refresh();
    fixture.Click(2);
    CHECK_FALSE(fixture.dialog.IsVisible());
    auto config = Menu(selected);
    config.buttons[0].onClick = [&]
    {
        auto reply = Menu(selected);
        reply.lines.insert(reply.lines.begin(), {L"已刷新当前转生条件。"});
        CHECK(fixture.dialog.ShowOrUpdate(std::move(reply)));
    };
    CHECK(fixture.dialog.ShowOrUpdate(config));
    fixture.layout.Refresh();
    fixture.Click(0);
    fixture.CheckFirstLine("已刷新当前转生条件");
    CHECK(fixture.document->IsVisible());
    fixture.Click(2);
    CHECK_FALSE(fixture.dialog.IsVisible());
}

TEST_CASE("Existing generic menu queue and final confirmation retain closing behavior [ui][reset-menu]")
{
    MenuFixture fixture("legacy", {1920, 1080}, 100);
    int selected = -1;
    auto config = Menu(selected);
    config.buttons[1].label = L"确认转生";
    config.buttons[1].closeOnClick = true;
    fixture.dialog.Show(config);
    auto next = Menu(selected);
    next.id = "next";
    next.lines = {{L"原有队列仍可使用"}};
    fixture.dialog.Show(next);
    fixture.layout.Refresh();
    fixture.Click(1);
    CHECK(selected == 1);
    CHECK(fixture.dialog.IsVisible());
    fixture.CheckFirstLine("原有队列仍可使用");
    fixture.Click(2);
    CHECK_FALSE(fixture.dialog.IsVisible());
}

TEST_CASE("Warp preview shows authoritative quotes before action including disabled destinations [ui][warp-preview]")
{
    MenuFixture fixture("legacy", {1920, 1080}, 100);
    int selected = -1;
    auto config = Menu(selected);
    config.id = "leveling-warp";
    config.buttons[0].label = L"冰风谷";
    config.buttons[0].description = L"雪人房1号";
    config.buttons[0].details = L"等级 20\n金币 2000";
    config.buttons[1].label = L"天空之城";
    config.buttons[1].description = L"红魔龙聚集点";
    config.buttons[1].details = L"等级 170\n金币 10000";
    config.buttons[1].enabled = false;
    REQUIRE(fixture.dialog.ShowOrUpdate(config));
    fixture.SelectDocument();
    auto* details = fixture.document->GetElementById("warp_preview_details");
    auto* name = fixture.document->GetElementById("warp_preview_name");
    REQUIRE(details);
    REQUIRE(name);
    CHECK(details->IsVisible(true));
    CHECK(details->GetInnerRML() == "等级 20\n金币 2000");
    Rml::ElementList buttons;
    fixture.document->QuerySelectorAll(buttons, ".gmd-btn");
    REQUIRE(buttons.size() >= 2);
    buttons[1]->DispatchEvent("mouseover", {});
    fixture.layout.Refresh();
    CHECK(details->GetInnerRML() == "等级 170\n金币 10000");
    CHECK(name->GetInnerRML().find("红魔龙聚集点") != Rml::String::npos);
    CHECK(selected == -1);
    buttons[1]->Click();
    fixture.dialog.Update();
    CHECK(selected == -1);
    buttons[0]->DispatchEvent("focus", {});
    fixture.layout.Refresh();
    CHECK(details->GetInnerRML() == "等级 20\n金币 2000");
    REQUIRE(fixture.dialog.ShowOrUpdate(config));
    fixture.layout.Refresh();
    CHECK(details->GetInnerRML() == "等级 20\n金币 2000");
    fixture.document->GetElementById("btn_exit")->Click();
    fixture.dialog.Update();
    CHECK_FALSE(fixture.dialog.IsVisible());
    REQUIRE(fixture.dialog.ShowOrUpdate(Menu(selected)));
    fixture.SelectDocument();
    CHECK_FALSE(fixture.document->GetElementById("warp_preview")->IsVisible(true));
}

TEST_CASE("Aria guide uses the fixed NPC frame and fits all three examples [ui][aria-guide]")
{
    for (const std::string theme : {"legacy", "classic", "modern"})
        for (const Rml::Vector2i viewport :
             {Rml::Vector2i{800, 600}, {1280, 720}, {1920, 1080}, {2560, 1440}, {3440, 1440}, {3840, 2160}})
            for (int scale : {75, 100, 125, 150})
            {
                CAPTURE(theme);
                CAPTURE(viewport.x);
                CAPTURE(viewport.y);
                CAPTURE(scale);
                MenuFixture fixture(theme, viewport, scale);
                CHARACTER aria{};
                aria.Object.Live = true;
                aria.Object.Kind = KIND_NPC;
                aria.MonsterIndex = MONSTER_CREATOR_GODDESS_ARIA;
                REQUIRE(UI::Npc::TryShowAriaGuide(aria));
                REQUIRE(UI::Npc::TryShowAriaGuide(aria));
                fixture.SelectDocument();
                fixture.CheckFixedPanel("创世神·艾莉雅");
                auto* body = fixture.document->GetElementById("npc_lines_container");
                REQUIRE(body);
                CHECK(body->GetScrollHeight() <= body->GetClientHeight() + 1.f);
                CHECK(body->GetScrollWidth() <= body->GetClientWidth() + 1.f);
                const auto content = body->GetInnerRML();
                CHECK(content.find("/Aria 剑士如何转职？") != Rml::String::npos);
                CHECK(content.find("/Aria 你好，我叫小明") != Rml::String::npos);
                CHECK(content.find("/Aria 洛克之羽打什么怪掉落？") != Rml::String::npos);
                CHECK(content.find("/aria、/艾莉雅、/小助理") != Rml::String::npos);
                CHECK(content.find("再按 Enter 发送。") != Rml::String::npos);
                CHECK_FALSE(fixture.document->GetElementById("answers_container")->IsVisible(true));
                CHECK_FALSE(fixture.document->GetElementById("warp_preview")->IsVisible(true));
                Rml::Vector2f bodyPosition, bodySize, closePosition, closeSize;
                auto* close = fixture.document->GetElementById("btn_exit");
                REQUIRE(close);
                REQUIRE(UI::RmlBridge::DrawnBox(*body, Rml::BoxArea::Border, bodyPosition, bodySize));
                REQUIRE(UI::RmlBridge::DrawnBox(*close, Rml::BoxArea::Border, closePosition, closeSize));
                CHECK(bodyPosition.y + bodySize.y <= closePosition.y + 1.f);
                close->Click();
                fixture.dialog.Update();
                CHECK_FALSE(fixture.dialog.IsVisible());
                CHECK_FALSE(fixture.document->IsVisible());
                REQUIRE(UI::Npc::TryShowAriaGuide(aria));
                fixture.SelectDocument();
                fixture.document->GetElementById("frame_corner_close")->Click();
                fixture.dialog.Update();
                CHECK_FALSE(fixture.dialog.IsVisible());
                // Closing the guide must allow an unrelated NPC to open immediately,
                // with no queued guide or duplicate page left behind.
                int selected = -1;
                REQUIRE(fixture.dialog.ShowOrUpdate(Menu(selected)));
                fixture.SelectDocument();
                fixture.CheckFirstLine("下一次");
                fixture.Click(2);
                CHECK_FALSE(fixture.dialog.IsVisible());
            }
}

TEST_CASE("Aria guide ignores unrelated speakers and preserves other active menus [ui][aria-guide]")
{
    MenuFixture fixture("legacy", {1920, 1080}, 100);
    CHARACTER aria{};
    aria.Object.Live = true;
    aria.Object.Kind = KIND_NPC;
    aria.MonsterIndex = MONSTER_CREATOR_GODDESS_ARIA;
    SUBCASE("different NPC")
    {
        aria.MonsterIndex = MONSTER_LEO_THE_HELPER;
        CHECK_FALSE(UI::Npc::TryShowAriaGuide(aria));
    }
    SUBCASE("dead object")
    {
        aria.Object.Live = false;
        CHECK_FALSE(UI::Npc::TryShowAriaGuide(aria));
    }
    SUBCASE("player cannot impersonate Aria")
    {
        aria.Object.Kind = KIND_PLAYER;
        CHECK_FALSE(UI::Npc::TryShowAriaGuide(aria));
    }
    SUBCASE("outside the game")
    {
        SceneFlag = LOG_IN_SCENE;
        CHECK_FALSE(UI::Npc::TryShowAriaGuide(aria));
    }
    SUBCASE("unavailable UI")
    {
        mu::ui::window::g_pGenericMenuDialog = nullptr;
        CHECK_FALSE(UI::Npc::TryShowAriaGuide(aria));
        mu::ui::window::g_pGenericMenuDialog = &fixture.dialog;
    }
    SUBCASE("unrelated menu stays open")
    {
        int selected = -1;
        REQUIRE(fixture.dialog.ShowOrUpdate(Menu(selected)));
        CHECK_FALSE(UI::Npc::TryShowAriaGuide(aria));
        fixture.SelectDocument();
        fixture.CheckFirstLine("下一次");
        fixture.Click(2);
    }
    CHECK_FALSE(fixture.dialog.IsVisible());
}
