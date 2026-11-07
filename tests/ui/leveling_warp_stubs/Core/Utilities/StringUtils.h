#pragma once
#include <codecvt>
#include <locale>
#include <string>
namespace StringUtils
{
inline std::wstring NarrowToWide(const std::string& text)
{
    return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(text);
}
} // namespace StringUtils
