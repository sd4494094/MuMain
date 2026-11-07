#pragma once
namespace mu::ui::window
{
struct GenericMenuConfig
{
    std::string id;
    std::wstring title;
    struct MenuButton
    {
        std::wstring label, description, details;
        bool enabled = true, compact = false, dismiss = false;
        std::function<void()> onClick;
    };
    std::vector<MenuButton> buttons;
    std::function<void()> onCancel;
};
struct CGenericMenuDialog
{
    bool visible = false;
    GenericMenuConfig active;
    bool IsVisible()
    {
        return visible;
    }
    void Show(GenericMenuConfig config)
    {
        active = std::move(config);
        visible = true;
    }
};
inline CGenericMenuDialog menu;
inline CGenericMenuDialog* g_pGenericMenuDialog = &menu;
} // namespace mu::ui::window
