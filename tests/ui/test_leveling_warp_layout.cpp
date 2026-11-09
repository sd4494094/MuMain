#include "stdafx.h"
#include <doctest.h>

#include "RmlLayoutFixture.h"
#include "UI/RmlBridge/RmlElementBox.h"
#include "UI/RmlBridge/RmlPanelGeometry.h"
#include "UI/Scaling/UITransform.h"
#include "UI/RmlBridge/RmlClickSounds.h"
#include "UI/RmlBridge/ThemeInheritance.h"
#include <RmlUi/Core/ElementScroll.h>
#include <RmlUi/Core/ElementText.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
struct Line
{
    Rml::String text;
    Rml::String color;
    bool bold = false;
};

struct Button
{
    Rml::String label;
    Rml::String description;
    Rml::String details = "等级 170    金币 10000";
    Rml::String tooltip;
    std::vector<Line> lines;
    bool enabled = true;
    bool compact = false;
    bool dismiss = false;
    bool other = false;
};

struct Menu
{
    Rml::String id = "leveling-warp";
    Rml::String kind;
    Rml::String title = "传送使者·诺亚";
    Rml::String close = "关闭";
    Rml::String previewName = "地下城一层 幽灵房", previewDetails = "等级 170    金币 10000";
    bool yes = true;
    bool no = false;
    float zero = 0.f, textPx = 12.f;
    std::vector<Line> lines;
    std::vector<Button> buttons;
    Menu()
    {
        // Real destination names include the longest map/location pairs and repeated map names.
        const std::vector<std::pair<Rml::String, Rml::String>> destinations = {
            {"地下城一层", "幽灵房"},        {"地下城一层", "一层尽头"},        {"地下城二层", "地牢"},
            {"地下城二层", "大厅"},          {"地下城二层", "毒牛房"},          {"失落之塔三层", "牛房"},
            {"失落之塔三层", "二号黄金点"},  {"失落之塔三层", "三号黄金点"},    {"失落之塔五层", "通往六层的入口"},
            {"失落之塔六层", "十字路口"},    {"失落之塔七层", "一号拐角"},      {"失落之塔七层", "大厅"},
            {"失落之塔七层", "二号拐角"},    {"亚特兰蒂斯一区", "五点练级区"},  {"亚特兰蒂斯", "浅海魔区"},
            {"死亡沙漠", "铁轮战士聚集点"},  {"死亡沙漠", "破坏骑士聚集点"},    {"死亡沙漠", "黑炎魔聚集点"},
            {"天空之城", "蓝魔龙聚集点"},    {"天空之城", "恶灵王聚集点"},      {"天空之城", "幻影骑士聚集点"},
            {"天空之城", "红魔龙聚集点"},    {"亚特兰蒂斯", "首领·海魔希特拉"}, {"死亡沙漠", "首领·魔王扎坎"},
            {"冰风谷", "雪人房1号"},         {"冰风谷", "雪人房2号"},           {"死亡沙漠", "首领·炽炎魔"},
            {"天空之城", "首领·天魔菲尼斯"},
        };
        for (const auto& [name, description] : destinations)
        {
            Button button;
            button.label = name;
            button.description = description;
            buttons.push_back(button);
        }
    }
    int selected = -1;
    int cancelled = 0;
};

void Bind(Rml::Context& context, Menu& menu)
{
    auto model = context.CreateDataModel("generic_menu_dialog");
    REQUIRE(model);
    auto line = model.RegisterStruct<Line>();
    line.RegisterMember("text", &Line::text);
    line.RegisterMember("color", &Line::color);
    line.RegisterMember("bold", &Line::bold);
    model.RegisterArray<std::vector<Line>>();
    auto button = model.RegisterStruct<Button>();
    button.RegisterMember("label", &Button::label);
    button.RegisterMember("description", &Button::description);
    button.RegisterMember("details", &Button::details);
    button.RegisterMember("tooltip", &Button::tooltip);
    button.RegisterMember("lines", &Button::lines);
    button.RegisterMember("enabled", &Button::enabled);
    button.RegisterMember("compact", &Button::compact);
    button.RegisterMember("dismiss", &Button::dismiss);
    for (const char* field : {"has_lines", "cols2", "narrow", "lines_below"})
        button.RegisterMember(field, &Button::other);
    model.RegisterArray<std::vector<Button>>();
    model.Bind("buttons", &menu.buttons);
    model.Bind("lines", &menu.lines);
    model.Bind("menu_id", &menu.id);
    model.Bind("kind", &menu.kind);
    model.Bind("title", &menu.title);
    model.Bind("preview_name", &menu.previewName);
    model.Bind("preview_details", &menu.previewDetails);
    model.Bind("close_label", &menu.close);
    model.Bind("text_px", &menu.textPx);
    model.Bind("system_menu_label", &menu.title);
    model.Bind("has_title", &menu.yes);
    for (const char* field : {"is_system_menu", "highlight_title", "positioned"})
        model.Bind(field, &menu.no);
    for (const char* field : {"canvas_top", "root_x", "root_y"})
        model.Bind(field, &menu.zero);
    model.BindEventCallback("gmd_button_click", [&menu](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
                            { menu.selected = args.at(0).Get<int>(); });
    model.BindEventCallback("gmd_preview",
                            [&menu](Rml::DataModelHandle handle, Rml::Event&, const Rml::VariantList& args)
                            {
                                const auto& button = menu.buttons.at(args.at(0).Get<int>());
                                if (button.dismiss)
                                    return;
                                menu.previewName = button.label + " " + button.description;
                                menu.previewDetails = button.details;
                                handle.DirtyVariable("preview_name");
                                handle.DirtyVariable("preview_details");
                            });
    model.BindEventCallback("gmd_cancel",
                            [&menu](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { ++menu.cancelled; });
}

bool Descendant(Rml::Element* child, Rml::Element* parent)
{
    for (; child; child = child->GetParentNode())
        if (child == parent)
            return true;
    return false;
}

void CheckHit(UI::Tests::RmlLayoutFixture& fixture, Rml::Element& element)
{
    Rml::Vector2f position, size;
    REQUIRE(UI::RmlBridge::DrawnBox(element, Rml::BoxArea::Border, position, size));
    CHECK(size.x > 0.f);
    CHECK(size.y > 0.f);
    CHECK(Descendant(fixture.context->GetElementAtPoint(position + size * 0.5f), &element));
}

void CheckLayout(UI::Tests::RmlLayoutFixture& fixture, Rml::ElementDocument& document, Menu& menu)
{
    auto* panel = document.GetElementById("panel");
    auto* pane = document.GetElementById("answers_container");
    auto* close = document.GetElementById("btn_exit");
    REQUIRE(panel);
    REQUIRE(pane);
    REQUIRE(close);
    Rml::Vector2f panelPosition, panelSize, panePosition, paneSize, closePosition, closeSize;
    REQUIRE(UI::RmlBridge::DrawnBox(*panel, Rml::BoxArea::Border, panelPosition, panelSize));
    REQUIRE(UI::RmlBridge::DrawnBox(*pane, Rml::BoxArea::Border, panePosition, paneSize));
    REQUIRE(UI::RmlBridge::DrawnBox(*close, Rml::BoxArea::Border, closePosition, closeSize));
    CHECK(panelPosition.x + panelSize.x == doctest::Approx(fixture.context->GetDimensions().x).epsilon(.005));
    CHECK_FALSE(panel->IsClassSet("center-both"));
    CHECK(panePosition.y + paneSize.y <= closePosition.y + 1.f);
    CHECK(pane->GetScrollHeight() > pane->GetClientHeight());
    CheckHit(fixture, *close);
    Rml::ElementList buttons;
    Rml::ElementList allButtons;
    pane->QuerySelectorAll(allButtons, ".gmd-btn");
    for (auto* button : allButtons)
        if (button->IsVisible(true))
            buttons.push_back(button);
    REQUIRE(buttons.size() == menu.buttons.size());
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        auto& button = *buttons[i];
        button.ScrollIntoView();
        fixture.Refresh();
        Rml::Vector2f position, size;
        REQUIRE(UI::RmlBridge::DrawnBox(button, Rml::BoxArea::Border, position, size));
        CHECK(size.y > 0.f);
        CHECK(position.x >= panePosition.x - 2.f);
        CHECK(position.x + size.x <= panePosition.x + paneSize.x + 2.f);
        CHECK(button.GetInnerRML().find(menu.buttons[i].label) != Rml::String::npos);
        CHECK(button.GetInnerRML().find(menu.buttons[i].description) != Rml::String::npos);
        CheckHit(fixture, button);
        button.DispatchEvent("mouseover", {});
        fixture.Refresh();
        auto* preview = document.GetElementById("warp_preview_details");
        REQUIRE(preview);
        CHECK(preview->IsVisible(true));
        if (!menu.buttons[i].dismiss)
            CHECK(preview->GetInnerRML() == menu.buttons[i].details);
        Rml::Vector2f previewPosition, previewSize;
        REQUIRE(UI::RmlBridge::DrawnBox(*preview, Rml::BoxArea::Border, previewPosition, previewSize));
        CHECK(previewSize.y > 0.f);
        CHECK(previewPosition.y + previewSize.y <= panePosition.y + 1.f);
        button.Click();
        CHECK(menu.selected == static_cast<int>(i));
        CheckHit(fixture, *close);
    }
    close->Click();
    CHECK(menu.cancelled == 1);
    pane->SetScrollTop(0.f);
    fixture.Refresh();
    CheckHit(fixture, *close);
    close->Click();
    CHECK(menu.cancelled == 2);
}
} // namespace

TEST_CASE("Leveling menu keeps all destinations and cancellation reachable at real UI scales [ui][leveling-layout]")
{
    for (const std::string theme : {"legacy", "classic", "modern"})
        for (const Rml::Vector2i viewport : {Rml::Vector2i{800, 600}, {1280, 720}, {1920, 1080}, {3440, 1440}})
            for (int scale : {75, 100, 125, 150})
            {
                CAPTURE(theme);
                CAPTURE(viewport.x);
                CAPTURE(viewport.y);
                CAPTURE(scale);
                UI::Tests::RmlLayoutFixture fixture;
                UI::RmlBridge::RegisterClickSoundProperty();
                REQUIRE(Rml::LoadFontFace(std::string(MU_FONT_DIR) + "/NotoSansSC-Regular.otf", true));
                fixture.Configure(viewport, scale);
                Menu menu;
                menu.textPx = UI::Scaling::NativeTextPixelSize(UI::Scaling::FontRole::Normal, viewport.x, viewport.y);
                Button dismiss;
                dismiss.label = "关闭";
                dismiss.description.clear();
                dismiss.details.clear();
                dismiss.compact = true;
                dismiss.dismiss = true;
                menu.buttons.push_back(dismiss);
                Bind(*fixture.context, menu);
                const auto path = std::filesystem::path(MU_RMLUI_DIR) / "themes" / theme / "npc_service_menu.rml";
                auto markupPath = path;
                if (!std::filesystem::exists(markupPath))
                    markupPath = std::filesystem::path(MU_RMLUI_DIR) / "npc_service_menu.rml";
                // Production loads shared RML from memory against the selected theme's virtual URL.
                auto* document = fixture.context->LoadDocumentFromMemory(UI::Tests::ReadFile(markupPath),
                                                                         UI::RmlBridge::ThemePathUtf8(path));
                REQUIRE(document);
                document->Show();
                const auto dock = UI::Scaling::DockRightTransform(viewport.x, viewport.y);
                UI::RmlBridge::SlotPlacement placement;
                document->UpdateDocument();
                const float width = document->GetElementById("panel")->GetBox().GetSize(Rml::BoxArea::Border).x;
                placement.Set(viewport.x - width * dock.scaleX, std::max(0.f, dock.offsetY), dock.scaleX);
                placement.Apply(document, "panel");
                fixture.Refresh();
                CheckLayout(fixture, *document, menu);
            }
}
