#include "stdafx.h"
#include <doctest.h>

#include "RmlLayoutFixture.h"
#include "UI/RmlBridge/RmlClickSounds.h"
#include "UI/RmlBridge/RmlElementBox.h"
#include "UI/RmlBridge/RmlPanelGeometry.h"
#include "UI/Scaling/UITransform.h"
#include "UI/RmlBridge/RmlNumericInputFilter.h"
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
    Rml::String action_label;
    int id = 0;
};

struct Model
{
    float textPx = 12.f;
    int page = 0, available_points = 5, selected_attribute = 0;
    Rml::String title = "洗点师·艾琳", message = "请填写本次增加的属性点数";
    Rml::String feedback, available = "可用点数：700", total = "合计：0", remaining = "剩余：700";
    Rml::String reset = "洗点", allocate = "增加", leave = "离开", back = "返回", clear = "清空";
    Rml::String next = "下一步", confirm_reset = "确认洗点", confirm_allocation = "确认加点";
    Rml::String attribute_label = "属性", current_label = "当前", increase_label = "增加", result_label = "加点后";
    bool busy = false;
    bool pending = false, valid = true, can_confirm = true;
    std::vector<Row> rows;
    int selected_action = 0;
};

void Bind(Rml::Context& context, Model& model)
{
    auto constructor = context.CreateDataModel("stat_advisor");
    REQUIRE(constructor);
    auto row = constructor.RegisterStruct<Row>();
    row.RegisterMember("id", &Row::id);
    row.RegisterMember("name", &Row::name);
    row.RegisterMember("action_label", &Row::action_label);
    row.RegisterMember("current", &Row::current);
    row.RegisterMember("amount", &Row::amount);
    row.RegisterMember("result", &Row::result);
    constructor.RegisterArray<std::vector<Row>>();
    constructor.Bind("text_px", &model.textPx);
    constructor.Bind("busy", &model.busy);
    constructor.Bind("selected_attribute", &model.selected_attribute);
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

TEST_CASE(
    "stat advisor single-attribute form and action buttons stay in bounds across themes and scales [ui][stat-advisor]")
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
                    model.textPx =
                        UI::Scaling::NativeTextPixelSize(UI::Scaling::FontRole::Normal, viewport.x, viewport.y);
                    const std::vector<std::string> names = {"力量", "敏捷", "体力", "智力", "统率"};
                    for (int i = 0; i < row_count; ++i)
                        model.rows.push_back({names[i], "100", "", "100", names[i] + "加点", i});
                    Bind(*fixture.context, model);
                    const auto path = std::filesystem::path(MU_RMLUI_DIR) / "themes" / theme / "stat_advisor.rml";
                    auto markupPath = path;
                    if (!std::filesystem::exists(markupPath))
                        markupPath = std::filesystem::path(MU_RMLUI_DIR) / "stat_advisor.rml";
                    auto* document = fixture.context->LoadDocumentFromMemory(UI::Tests::ReadFile(markupPath),
                                                                              UI::RmlBridge::ThemePathUtf8(path));
                    REQUIRE(document);
                    UI::RmlBridge::AttachNumericInputFilter(document);
                    document->Show();
                    const auto dock = UI::Scaling::DockRightTransform(viewport.x, viewport.y);
                    UI::RmlBridge::SlotPlacement placement;
                    document->UpdateDocument();
                    const float width = document->GetElementById("panel")->GetBox().GetSize(Rml::BoxArea::Border).x;
                    placement.Set(viewport.x - width * dock.scaleX, std::max(0.f, dock.offsetY), dock.scaleX);
                    placement.Apply(document, "panel");
                    fixture.Refresh();
                    auto* message = document->QuerySelector(".advisor-message");
                    REQUIRE(message);
                    CHECK(message->IsVisible(true));
                    Rml::Vector2f messagePosition, messageSize;
                    REQUIRE(UI::RmlBridge::DrawnBox(*message, Rml::BoxArea::Border, messagePosition, messageSize));
                    CHECK(messageSize.y > 0.f);
                    auto handle = fixture.context->GetDataModel("stat_advisor").GetModelHandle();
                    Rml::ElementList actions, allActions;
                    document->QuerySelectorAll(allActions, ".advisor-action");
                    for (auto* action : allActions)
                        if (action->IsVisible(true))
                            actions.push_back(action);
                    REQUIRE(actions.size() == static_cast<size_t>(row_count + 1));
                    for (int i = 0; i < row_count; ++i)
                    {
                        CHECK(actions[i + 1]->GetInnerRML().find(std::to_string(i + 2) + ".") != Rml::String::npos);
                        CHECK(actions[i + 1]->GetInnerRML().find(names[i] + "加点") != Rml::String::npos);
                    }
                    for (int page : {2, 3, 2, 1, 0, 2})
                    {
                        model.page = page;
                        handle.DirtyVariable("page");
                        fixture.Refresh();
                        CHECK(message->IsVisible(true));
                        auto* form = document->QuerySelector(".advisor-form");
                        REQUIRE(form);
                        CHECK(form->IsVisible(true) == (page == 2 || page == 3));
                    }

                    Rml::ElementList inputs;
                    document->QuerySelectorAll(inputs, ".advisor-input");
                    auto visibleInput =
                        std::find_if(inputs.begin(), inputs.end(), [](auto* input) { return input->IsVisible(true); });
                    REQUIRE(visibleInput != inputs.end());
                    auto* firstInput = *visibleInput;
                    REQUIRE(firstInput->Focus());
                    fixture.context->ProcessTextInput("9");
                    fixture.Refresh();
                    CHECK(model.rows.front().amount == "9");
                    fixture.context->ProcessTextInput("x");
                    fixture.Refresh();
                    CHECK(model.rows.front().amount == "9");

                    auto* panel = document->GetElementById("panel");
                    auto* body = document->GetElementById("advisor-options");
                    auto* footer = document->QuerySelector(".advisor-footer");
                    REQUIRE(panel);
                    REQUIRE(body);
                    Rml::Vector2f bodyPosition, bodySize;
                    REQUIRE(UI::RmlBridge::DrawnBox(*body, Rml::BoxArea::Border, bodyPosition, bodySize));
                    CHECK(bodySize.y > 100.f);
                    CHECK(firstInput->IsVisible(true));
                    Rml::Vector2f inputPosition, inputSize;
                    REQUIRE(UI::RmlBridge::DrawnBox(*firstInput, Rml::BoxArea::Border, inputPosition, inputSize));
                    CHECK(inputPosition.y + inputSize.y <= bodyPosition.y + bodySize.y + 1.f);
                    REQUIRE(footer);
                    Rml::Vector2f panelPosition, panelSize, footerPosition, footerSize;
                    REQUIRE(UI::RmlBridge::DrawnBox(*panel, Rml::BoxArea::Border, panelPosition, panelSize));
                    REQUIRE(UI::RmlBridge::DrawnBox(*footer, Rml::BoxArea::Border, footerPosition, footerSize));
                    const auto dimensions = fixture.context->GetDimensions();
                    CHECK(panelPosition.x == doctest::Approx(viewport.x - panelSize.x).epsilon(0.005));
                    CHECK_FALSE(panel->IsClassSet("center-both"));
                    CHECK_FALSE(panel->IsClassSet("dragged"));
                    CHECK(panelPosition.x >= -2.f);
                    CHECK(panelPosition.y >= -2.f);
                    CHECK(panelPosition.x + panelSize.x <= dimensions.x + 2.f);
                    CHECK(panelPosition.y + panelSize.y <= dimensions.y + 2.f);
                    CHECK(footerPosition.y >= panelPosition.y);
                    CHECK(footerPosition.y + footerSize.y <= panelPosition.y + panelSize.y + 2.f);

                    Rml::ElementList rows;
                    document->QuerySelectorAll(rows, ".advisor-row");

                    Rml::ElementList fields;
                    body->QuerySelectorAll(fields, ".advisor-input, .advisor-summary");

                    for (auto* field : fields)
                    {
                        if (!field->IsVisible(true))
                            continue;
                        Rml::Vector2f pos, size;
                        REQUIRE(UI::RmlBridge::DrawnBox(*field, Rml::BoxArea::Border, pos, size));
                        CHECK(size.y > 0.f);
                        CHECK(pos.x >= bodyPosition.x - 1.f);
                        CHECK(pos.x + size.x <= bodyPosition.x + bodySize.x + 1.f);
                        CHECK(pos.y >= bodyPosition.y - 1.f);
                        CHECK(pos.y + size.y <= bodyPosition.y + bodySize.y + 1.f);
                    }
                    for (int selected = 0; selected < row_count; ++selected)
                    {
                        model.page = 0;
                        handle.DirtyVariable("page");
                        fixture.Refresh();
                        Rml::ElementList choices;
                        document->QuerySelectorAll(choices, ".advisor-home .advisor-action");
                        REQUIRE(UI::RmlBridge::DrawnBox(*body, Rml::BoxArea::Border, bodyPosition, bodySize));
                        int visibleChoices = 0;
                        for (auto* choice : choices)
                        {
                            if (!choice->IsVisible(true))
                                continue;
                            ++visibleChoices;
                            Rml::Vector2f pos, size;
                            REQUIRE(UI::RmlBridge::DrawnBox(*choice, Rml::BoxArea::Border, pos, size));
                            CHECK(pos.y + size.y <= bodyPosition.y + bodySize.y + 1.f);
                            choice->Click();
                            if (visibleChoices > 1)
                                CHECK(model.selected_action == 8 + visibleChoices);
                        }
                        CHECK(visibleChoices == row_count + 1);
                        model.page = 2;
                        model.selected_attribute = selected;
                        handle.DirtyVariable("page");
                        handle.DirtyVariable("selected_attribute");
                        fixture.Refresh();
                        inputs.clear();
                        document->QuerySelectorAll(inputs, ".advisor-input");
                        int visibleInputs = 0;
                        for (auto* input : inputs)
                        {
                            if (!input->IsVisible(true))
                                continue;
                            ++visibleInputs;
                            REQUIRE(input->Focus());
                            fixture.context->ProcessTextInput("1");
                            fixture.Refresh();
                        }
                        CHECK(visibleInputs == 1);
                        CHECK(model.rows[selected].amount == (selected == 0 ? "91" : "1"));
                    }
                    Rml::ElementList buttons;
                    footer->QuerySelectorAll(buttons, ".nd-answer-row");
                    Rml::ElementList visibleButtons;
                    for (auto* button : buttons)
                        if (button->GetComputedValues().display() != Rml::Style::Display::None)
                            visibleButtons.push_back(button);
                    REQUIRE(visibleButtons.size() == 3);
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
