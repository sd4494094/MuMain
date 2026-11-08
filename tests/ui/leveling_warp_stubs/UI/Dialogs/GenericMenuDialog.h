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
        bool enabled = true, compact = false, dismiss = false, closeOnClick = true;
        std::function<void()> onClick;
    };
    struct Line
    {
        std::wstring text;
    };
    std::vector<Line> lines;
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
    bool ShowOrUpdate(GenericMenuConfig config)
    {
        if (visible && (config.id.empty() || active.id != config.id))
            return false;
        Show(std::move(config));
        return true;
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
