#include "stdafx.h"
#include <doctest.h>

#include "RmlLayoutFixture.h"
#include "UI/RmlBridge/RmlElementBox.h"
#include "UI/RmlBridge/RmlClickSounds.h"
#include "UI/RmlBridge/ThemeInheritance.h"
#include <RmlUi/Core/ElementScroll.h>
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
    Rml::String label = "失落之塔七层";
    Rml::String description = "通往六层的入口";
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
    Rml::String title = "练级点传送";
    Rml::String close = "关闭";
    bool yes = true;
    bool no = false;
    float zero = 0.f;
    std::vector<Line> lines;
    std::vector<Button> buttons{26};
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
    model.Bind("close_label", &menu.close);
    model.Bind("system_menu_label", &menu.title);
    model.Bind("has_title", &menu.yes);
    for (const char* field : {"is_system_menu", "highlight_title", "positioned"})
        model.Bind(field, &menu.no);
    for (const char* field : {"canvas_top", "root_x", "root_y"})
        model.Bind(field, &menu.zero);
    model.BindEventCallback("gmd_button_click", [&menu](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
                            { menu.selected = args.at(0).Get<int>(); });
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
    constexpr float Tolerance = 2.f;
    auto* pane = document.QuerySelector(".gmd-button-row");
    auto* close = document.QuerySelector(".gmd-leveling-close");
    REQUIRE(pane);
    REQUIRE(close);
    Rml::Vector2f panelPosition, panelSize, panePosition, paneSize, closePosition, closeSize;
    REQUIRE(UI::RmlBridge::DrawnBox(document, Rml::BoxArea::Border, panelPosition, panelSize));
    REQUIRE(UI::RmlBridge::DrawnBox(*pane, Rml::BoxArea::Border, panePosition, paneSize));
    REQUIRE(UI::RmlBridge::DrawnBox(*close, Rml::BoxArea::Border, closePosition, closeSize));
    const auto viewport = fixture.context->GetDimensions();
    CHECK(panelPosition.x >= -Tolerance);
    CHECK(panelPosition.y >= -Tolerance);
    CHECK(panelPosition.x + panelSize.x <= viewport.x + Tolerance);
    CHECK(panelPosition.y + panelSize.y <= viewport.y + Tolerance);
    CHECK(panePosition.y + paneSize.y <= closePosition.y + Tolerance);
    CHECK(closePosition.y + closeSize.y <= panelPosition.y + panelSize.y);
    CHECK(pane->GetScrollHeight() > pane->GetClientHeight());
    CheckHit(fixture, *close);

    Rml::ElementList buttons;
    pane->QuerySelectorAll(buttons, ".gmd-button-cell:not(.dismiss) .gmd-btn");
    REQUIRE(buttons.size() == 26);
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        auto& button = *buttons[i];
        const float top = button.GetAbsoluteOffset(Rml::BoxArea::Border).y -
                          pane->GetAbsoluteOffset(Rml::BoxArea::Content).y + pane->GetScrollTop();
        pane->SetScrollTop(top);
        fixture.Refresh();
        Rml::Vector2f position, size;
        REQUIRE(UI::RmlBridge::DrawnBox(button, Rml::BoxArea::Border, position, size));
        CHECK(position.x >= panePosition.x - Tolerance);
        CHECK(position.x + size.x <= panePosition.x + paneSize.x + Tolerance);
        CHECK(position.y >= panePosition.y - Tolerance);
        CHECK(position.y + size.y <= panePosition.y + paneSize.y + Tolerance);
        // Long names and requirements must remain within their own row.
        for (const char* selector : {".gmd-btn-label", ".gmd-btn-description", ".gmd-btn-details"})
        {
            auto* text = button.QuerySelector(selector);
            REQUIRE(text);
            Rml::Vector2f textPosition, textSize;
            REQUIRE(UI::RmlBridge::DrawnBox(*text, Rml::BoxArea::Border, textPosition, textSize));
            CHECK(textPosition.y >= position.y - Tolerance);
            CHECK(textPosition.y + textSize.y <= position.y + size.y + Tolerance);
        }
        CheckHit(fixture, button);
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
                Button dismiss;
                dismiss.label = "关闭";
                dismiss.description.clear();
                dismiss.details.clear();
                dismiss.compact = true;
                dismiss.dismiss = true;
                menu.buttons.push_back(dismiss);
                Bind(*fixture.context, menu);
                const auto path = std::filesystem::path(MU_RMLUI_DIR) / "themes" / theme / "generic_menu_dialog.rml";
                auto markupPath = path;
                if (!std::filesystem::exists(markupPath))
                    markupPath = std::filesystem::path(MU_RMLUI_DIR) / "generic_menu_dialog.rml";
                // Production loads shared RML from memory against the selected theme's virtual URL.
                auto* document = fixture.context->LoadDocumentFromMemory(UI::Tests::ReadFile(markupPath),
                                                                         UI::RmlBridge::ThemePathUtf8(path));
                REQUIRE(document);
                document->Show();
                fixture.Refresh();
                CheckLayout(fixture, *document, menu);
            }
}
