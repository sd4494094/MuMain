#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace Character
{
// Every legacy character-name field is ten UTF-8 bytes, including chat and parties.
inline constexpr std::size_t MaximumNameBytes = 10;
inline constexpr std::size_t MinimumNameCharacters = 3;

// Editing clips complete UTF-8 characters; submission validates without truncating.
inline std::string LimitNameBytes(std::string_view value)
{
    std::size_t end = value.size() < MaximumNameBytes ? value.size() : MaximumNameBytes;
    while (end > 0 && end < value.size() && (static_cast<unsigned char>(value[end]) & 0xC0) == 0x80)
        --end;
    return std::string(value.substr(0, end));
}

inline bool IsValidName(std::string_view value)
{
    if (value.empty() || value.size() > MaximumNameBytes)
        return false;

    std::size_t characters = 0;
    for (std::size_t offset = 0; offset < value.size(); ++characters)
    {
        const auto first = static_cast<unsigned char>(value[offset]);
        const bool ascii =
            (first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z') || (first >= '0' && first <= '9');
        if (ascii)
        {
            ++offset;
            continue;
        }

        // Season Six permits BMP Han characters, not symbols or supplementary characters.
        if (first < 0xE0 || first > 0xEF || offset + 2 >= value.size())
            return false;
        const auto second = static_cast<unsigned char>(value[offset + 1]);
        const auto third = static_cast<unsigned char>(value[offset + 2]);
        if ((second & 0xC0) != 0x80 || (third & 0xC0) != 0x80)
            return false;
        const unsigned int codepoint = ((first & 0x0F) << 12) | ((second & 0x3F) << 6) | (third & 0x3F);
        const bool han = (codepoint >= 0x3400 && codepoint <= 0x4DBF) || (codepoint >= 0x4E00 && codepoint <= 0x9FFF);
        if (!han)
            return false;
        offset += 3;
    }
    return characters >= MinimumNameCharacters;
}
} // namespace Character
