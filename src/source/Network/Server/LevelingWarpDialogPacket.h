#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Network
{
struct LevelingWarpEntry
{
    int number = 0;
    std::string name;
    std::string description;
    int level = 0;
    int costs = 0;
    bool enabled = false;
};

// OpenMU ServerToClientPackets.xml, F2/02. All text is display-only UTF-8.
struct LevelingWarpDialogPacket
{
    std::string token;
    std::string title;
    std::string closeLabel;
    std::string levelLabel;
    std::string costLabel;
    std::vector<LevelingWarpEntry> entries;

    static std::optional<LevelingWarpDialogPacket> Parse(std::span<const std::uint8_t> packet);
};
} // namespace Network
