#include "Network/Server/LevelingWarpDialogPacket.h"

#include <json.hpp>
#include <algorithm>
#include <array>
#include <set>
#include <limits>

namespace
{
constexpr std::size_t HeaderSize = 5;
constexpr std::size_t JsonOffset = 262;
constexpr std::size_t MaximumPacketSize = 60000;
constexpr std::size_t MaximumEntries = 64;
constexpr std::size_t MaximumNameBytes = 96;
constexpr std::size_t MaximumDescriptionBytes = 160;

std::optional<std::string> ReadString(std::span<const std::uint8_t> bytes)
{
    const auto end = std::find(bytes.begin(), bytes.end(), 0);
    if (end == bytes.end())
        return std::nullopt;
    return std::string(bytes.begin(), end);
}

bool IsToken(const std::string& token)
{
    constexpr std::size_t TokenDigits = 32;
    return token.size() == TokenDigits && std::all_of(token.begin(), token.end(), [](char c)
                                                      { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}

bool ReadEntries(const std::string& text, std::vector<Network::LevelingWarpEntry>& entries)
{
    const auto json = nlohmann::json::parse(text, nullptr, false);
    if (json.is_discarded() || !json.is_array() || json.empty() || json.size() > MaximumEntries)
        return false;
    std::set<int> numbers;
    try
    {
        for (const auto& row : json)
        {
            if (!row.is_object() || !row.at("Number").is_number_integer() || !row.at("Name").is_string() ||
                !row.at("Description").is_string() || !row.at("Level").is_number_integer() ||
                !row.at("Costs").is_number_integer() || !row.at("Enabled").is_boolean())
                return false;
            if (row.at("Number") < 1 || row.at("Number") > UINT16_MAX || row.at("Level") < 0 ||
                row.at("Level") > UINT16_MAX || row.at("Costs") < 0 ||
                row.at("Costs") > std::numeric_limits<int>::max())
                return false;
            Network::LevelingWarpEntry entry{row.at("Number").get<int>(),
                                             row.at("Name").get<std::string>(),
                                             row.at("Description").get<std::string>(),
                                             row.at("Level").get<int>(),
                                             row.at("Costs").get<int>(),
                                             row.at("Enabled").get<bool>()};
            if (entry.number <= 0 || entry.number > UINT16_MAX || !numbers.insert(entry.number).second ||
                entry.name.empty() || entry.name.size() > MaximumNameBytes ||
                entry.name.find(char{0}) != std::string::npos || entry.description.find(char{0}) != std::string::npos ||
                entry.description.empty() || entry.description.size() > MaximumDescriptionBytes || entry.level < 0 ||
                entry.level > UINT16_MAX || entry.costs < 0 || entry.name.find_first_of("\r\n") != std::string::npos ||
                entry.description.find_first_of("\r\n") != std::string::npos)
                return false;
            entries.push_back(std::move(entry));
        }
    }
    catch (const nlohmann::json::exception&)
    {
        return false;
    }
    return true;
}
} // namespace

namespace Network
{
std::optional<LevelingWarpDialogPacket> LevelingWarpDialogPacket::Parse(std::span<const std::uint8_t> packet)
{
    if (packet.size() <= JsonOffset || packet.size() > MaximumPacketSize || packet[0] != 0xC2 || packet[3] != 0xF2 ||
        packet[4] != 0x02 || ((static_cast<std::size_t>(packet[1]) << 8) | packet[2]) != packet.size())
        return std::nullopt;
    const std::array<std::size_t, 6> lengths{33, 96, 64, 32, 32, packet.size() - JsonOffset};
    std::array<std::string, 6> values;
    auto offset = HeaderSize;
    for (std::size_t i = 0; i < lengths.size(); ++i)
    {
        auto value = ReadString(packet.subspan(offset, lengths[i]));
        if (!value)
            return std::nullopt;
        values[i] = std::move(*value);
        offset += lengths[i];
    }
    if (!IsToken(values[0]))
        return std::nullopt;
    LevelingWarpDialogPacket result{values[0], values[1], values[2], values[3], values[4], {}};
    if (!ReadEntries(values[5], result.entries))
        return std::nullopt;
    return result;
}
} // namespace Network
