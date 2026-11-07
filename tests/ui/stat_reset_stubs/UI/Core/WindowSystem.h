#pragma once
namespace mu::ui::window
{
inline constexpr int TYPE_ERROR_MESSAGE = 1;
}
struct StatResetTestLog
{
    int errors = 0;
    void AddText(const wchar_t*, int)
    {
        ++errors;
    }
};
inline StatResetTestLog statResetTestLog;
inline StatResetTestLog* g_pSystemLogBox = &statResetTestLog;

inline int refiningTestWindow = -1;
namespace mu::ui::window
{
inline constexpr int INTERFACE_MIXINVENTORY = 17;
}
namespace UI::Windows
{
inline void Show(int window)
{
    refiningTestWindow = window;
}
} // namespace UI::Windows
