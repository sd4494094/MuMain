#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>

// A headless host: production StatResetDialogue.cpp builds the actual callbacks.
// Rendering and input dispatch remain the responsibility of GenericMenuDialog.
namespace mu::ui::window
{
struct GenericMenuConfig
{
    std::string id;
    std::string kind;
    std::wstring title;
    struct Line
    {
        std::wstring text;
        bool bold = false;
    };
    struct MenuButton
    {
        std::wstring label;
        bool compact = false;
        bool dismiss = false;
        std::function<void()> onClick;
    };
    std::vector<Line> lines;
    std::vector<MenuButton> buttons;
    std::function<void()> onCancel;
};
struct CGenericMenuDialog
{
    GenericMenuConfig active;
    void Show(GenericMenuConfig config)
    {
        active = std::move(config);
    }
};
inline CGenericMenuDialog statResetTestMenu;
inline CGenericMenuDialog* g_pGenericMenuDialog = &statResetTestMenu;
} // namespace mu::ui::window
