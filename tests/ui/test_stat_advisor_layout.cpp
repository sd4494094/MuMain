#include "stdafx.h"
#include <doctest.h>

#include "RmlLayoutFixture.h"
#include "UI/RmlBridge/RmlClickSounds.h"
#include "UI/RmlBridge/RmlElementBox.h"
#include "UI/RmlBridge/ThemeInheritance.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
struct Row
{
    Rml::String name, current, amount, result;
};

struct Model
{
    int page = 2, available_points = 700;
    Rml::String title = "洗点师·艾琳", message = "请填写本次增加的属性点数";
    Rml::String feedback, available = "可用点数：700", total = "合计：0", remaining = "剩余：700";
    Rml::String reset = "洗点", allocate = "批量加点", leave = "离开", back = "返回", clear = "清空";
    Rml::String next = "下一步", confirm_reset = "确认洗点", confirm_allocation = "确认批量加点";
    Rml::String attribute_label = "属性", current_label = "当前", increase_label = "增加", result_label = "加点后";
    bool pending = false, valid = true, can_confirm = true;
    std::vector<Row> rows;
    int selected_action = 0;
};

void Bind(Rml::Context& context, Model& model)
{
    auto constructor = context.CreateDataModel("stat_advisor");
    REQUIRE(constructor);
    auto row = constructor.RegisterStruct<Row>();
    row.RegisterMember("name", &Row::name);
    row.RegisterMember("current", &Row::current);
    row.RegisterMember("amount", &Row::amount);
    row.RegisterMember("result", &Row::result);
    constructor.RegisterArray<std::vector<Row>>();
    for (const auto* name : {"rows", "page", "title", "message", "feedback", "available", "total", "remaining",
                             "pending", "valid", "can_confirm", "reset", "allocate", "leave", "back", "clear", "next",
                             "confirm_reset", "confirm_allocation", "attribute_label", "current_label", "increase_label", "result_label"})
    {
        if (std::string(name) == "rows")
            constructor.Bind(name, &model.rows);
        else if (std::string(name) == "page")
            constructor.Bind(name, &model.page);
        else if (std::string(name) == "pending")
            constructor.Bind(name, &model.pending);
        else if (std::string(name) == "valid")
            constructor.Bind(name, &model.valid);
        else if (std::string(name) == "can_confirm")
            constructor.Bind(name, &model.can_confirm);
        else
        {
            auto bind = [&](auto* field) { constructor.Bind(name, field); };
            if (std::string(name) == "title") bind(&model.title);
            else if (std::string(name) == "message") bind(&model.message);
            else if (std::string(name) == "feedback") bind(&model.feedback);
            else if (std::string(name) == "available") bind(&model.available);
            else if (std::string(name) == "total") bind(&model.total);
            else if (std::string(name) == "remaining") bind(&model.remaining);
            else if (std::string(name) == "reset") bind(&model.reset);
            else if (std::string(name) == "allocate") bind(&model.allocate);
            else if (std::string(name) == "leave") bind(&model.leave);
            else if (std::string(name) == "back") bind(&model.back);
            else if (std::string(name) == "clear") bind(&model.clear);
            else if (std::string(name) == "next") bind(&model.next);
            else if (std::string(name) == "confirm_reset") bind(&model.confirm_reset);
            else if (std::string(name) == "confirm_allocation") bind(&model.confirm_allocation);
            else if (std::string(name) == "attribute_label") bind(&model.attribute_label);
            else if (std::string(name) == "current_label") bind(&model.current_label);
            else if (std::string(name) == "increase_label") bind(&model.increase_label);
            else if (std::string(name) == "result_label") bind(&model.result_label);
        }
    }
    constructor.BindEventCallback("advisor_action", [&model](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
                                  { model.selected_action = args.at(0).Get<int>(); });
}
} // namespace

TEST_CASE("stat advisor allocation table and action buttons stay in bounds across themes and scales [ui][stat-advisor]")
{
    for (const std::string theme : {"legacy", "classic", "modern"})
        for (const Rml::Vector2i viewport : {Rml::Vector2i{800, 600}, {1280, 720}, {1920, 1080}})
            for (const int scale : {75, 100, 125, 150})
                for (const int row_count : {4, 5})
                {
                    CAPTURE(theme);
                    CAPTURE(viewport.x);
                    CAPTURE(viewport.y);
                    CAPTURE(scale);
                    CAPTURE(row_count);
                    UI::Tests::RmlLayoutFixture fixture;
                    UI::RmlBridge::RegisterClickSoundProperty();
                    REQUIRE(Rml::LoadFontFace(std::string(MU_FONT_DIR) + "/NotoSansSC-Regular.otf", true));
                    fixture.Configure(viewport, scale);
                    Model model;
                    const std::vector<std::string> names = {"力量", "敏捷", "体力", "智力", "统率"};
                    for (int i = 0; i < row_count; ++i)
                        model.rows.push_back({names[i], "100", "25", "125"});
                    Bind(*fixture.context, model);
                    const auto path = std::filesystem::path(MU_RMLUI_DIR) / "themes" / theme / "stat_advisor.rml";
                    auto markupPath = path;
                    if (!std::filesystem::exists(markupPath))
                        markupPath = std::filesystem::path(MU_RMLUI_DIR) / "stat_advisor.rml";
                    auto* document = fixture.context->LoadDocumentFromMemory(UI::Tests::ReadFile(markupPath),
                                                                              UI::RmlBridge::ThemePathUtf8(path));
                    REQUIRE(document);
                    document->Show();
                    fixture.Refresh();

                    auto* panel = document->GetElementById("panel");
                    auto* body = document->GetElementById("advisor-body");
                    auto* footer = document->QuerySelector(".advisor-footer");
                    REQUIRE(panel);
                    REQUIRE(body);
                    REQUIRE(footer);
                    Rml::Vector2f panelPosition, panelSize, footerPosition, footerSize;
                    REQUIRE(UI::RmlBridge::DrawnBox(*panel, Rml::BoxArea::Border, panelPosition, panelSize));
                    REQUIRE(UI::RmlBridge::DrawnBox(*footer, Rml::BoxArea::Border, footerPosition, footerSize));
                    const auto dimensions = fixture.context->GetDimensions();
                    CHECK(panelPosition.x >= -2.f);
                    CHECK(panelPosition.y >= -2.f);
                    CHECK(panelPosition.x + panelSize.x <= dimensions.x + 2.f);
                    CHECK(panelPosition.y + panelSize.y <= dimensions.y + 2.f);
                    CHECK(footerPosition.y >= panelPosition.y);
                    CHECK(footerPosition.y + footerSize.y <= panelPosition.y + panelSize.y + 2.f);

                    Rml::ElementList rows;
                    document->QuerySelectorAll(rows, ".advisor-row");
                    REQUIRE(rows.size() == static_cast<size_t>(row_count + 2));
                    Rml::ElementList buttons;
                    footer->QuerySelectorAll(buttons, ".btn");
                    Rml::ElementList visibleButtons;
                    for (auto* button : buttons)
                        if (button->GetComputedValues().display() != Rml::Style::Display::None)
                            visibleButtons.push_back(button);
                    REQUIRE(visibleButtons.size() == 4);
                    for (auto* button : visibleButtons)
                    {
                        Rml::Vector2f position, size;
                        REQUIRE(UI::RmlBridge::DrawnBox(*button, Rml::BoxArea::Border, position, size));
                        CHECK(position.x >= panelPosition.x);
                        CHECK(position.x + size.x <= panelPosition.x + panelSize.x + 2.f);
                        CHECK(position.y >= footerPosition.y);
                        CHECK(position.y + size.y <= footerPosition.y + footerSize.y + 2.f);
                        button->Click();
                        CHECK(model.selected_action != 0);
                        model.selected_action = 0;
                    }
                }
}
