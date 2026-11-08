#include "stdafx.h"
#include <doctest.h>
#include "RmlLayoutFixture.h"
#include "UI/Core/WindowManager.h"
#include "UI/Dialogs/GenericMenuDialog.h"
#include "UI/RmlBridge/RmlClickSounds.h"
#include "UI/RmlBridge/RmlElementBox.h"
#include "UI/RmlBridge/RmlTheme.h"
#include <filesystem>

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
        dialog.Release();
        manager.RemoveUIObj(&dialog);
        UI::RmlBridge::SetContextResolver({});
        std::filesystem::current_path(previousDirectory);
        UI::RmlBridge::SetActiveThemeName(previousTheme);
    }
    void Click(int index)
    {
        Rml::ElementList buttons;
        document->QuerySelectorAll(buttons, ".gmd-btn");
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
    void CheckFirstLine(const std::string& text)
    {
        layout.Refresh();
        auto* body = document->QuerySelector(".gmd-lines");
        REQUIRE(body);
        auto* line = body->QuerySelector(".gmd-line");
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

    UI::Tests::RmlLayoutFixture layout;
    std::filesystem::path previousDirectory;
    std::string previousTheme;
    mu::ui::window::CManager manager;
    mu::ui::window::CGenericMenuDialog dialog;
    Rml::ElementDocument* document = nullptr;
};

GenericMenuConfig Menu(int& selected)
{
    GenericMenuConfig config;
    config.id = "reset-npc";
    config.title = L"转生导师·里奥";
    config.lines = {{L"下一次：第1次转生；当前375级，需要400级。"},
                    {L"需要金币：1；当前持有：180214647。"},
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
    fixture.layout.Refresh();
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
