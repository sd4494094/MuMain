#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace Network
{
// F2/01 is a MuMain-only server packet. The wire definition is
// OpenMU/src/Network/Packets/ServerToClient/ServerToClientPackets.xml.
struct ResetNpcDialogPacket
{
    std::string confirmationToken;
    std::string title;
    std::string queryLabel;
    std::string applyLabel;
    std::string closeLabel;
    std::string text;

    static std::optional<ResetNpcDialogPacket> Parse(std::span<const std::uint8_t> packet);
};
} // namespace Network
