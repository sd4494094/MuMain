#include "Network/Server/ResetNpcDialogPacket.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

namespace
{
constexpr std::size_t HeaderSize = 5;
constexpr std::size_t TokenSize = 33;
constexpr std::size_t TitleSize = 96;
constexpr std::size_t LabelSize = 64;
constexpr std::size_t TextOffset = HeaderSize + TokenSize + TitleSize + 3 * LabelSize;
constexpr std::size_t MaximumPacketSize = 8192;
constexpr std::uint8_t DialogCode = 0xF2;
constexpr std::uint8_t DialogSubCode = 0x01;

std::optional<std::string> ReadString(std::span<const std::uint8_t> bytes)
{
    const auto end = std::find(bytes.begin(), bytes.end(), 0);
    if (end == bytes.end())
        return std::nullopt;
    return std::string(bytes.begin(), end);
}

bool IsConfirmationToken(std::string_view token)
{
    constexpr std::size_t TokenDigits = TokenSize - 1;
    if (token.empty())
        return true;
    return token.size() == TokenDigits && std::all_of(token.begin(), token.end(), [](char c)
                                                      { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}
} // namespace

namespace Network
{
std::optional<ResetNpcDialogPacket> ResetNpcDialogPacket::Parse(std::span<const std::uint8_t> packet)
{
    if (packet.size() <= TextOffset || packet.size() > MaximumPacketSize)
        return std::nullopt;
    const auto declaredLength = (static_cast<std::size_t>(packet[1]) << 8) | packet[2];
    if (packet[0] != 0xC2 || packet[3] != DialogCode || packet[4] != DialogSubCode || declaredLength != packet.size())
        return std::nullopt;

    const std::array<std::size_t, 6> sizes{TokenSize, TitleSize, LabelSize,
                                           LabelSize, LabelSize, packet.size() - TextOffset};
    std::array<std::string, 6> values;
    std::size_t offset = HeaderSize;
    for (std::size_t i = 0; i < sizes.size(); ++i)
    {
        auto value = ReadString(packet.subspan(offset, sizes[i]));
        if (!value)
            return std::nullopt;
        values[i] = std::move(*value);
        offset += sizes[i];
    }
    if (!IsConfirmationToken(values[0]))
        return std::nullopt;
    return ResetNpcDialogPacket{values[0], values[1], values[2], values[3], values[4], values[5]};
}
} // namespace Network
