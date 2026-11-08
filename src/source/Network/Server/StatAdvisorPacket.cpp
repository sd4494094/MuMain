#include "Network/Server/StatAdvisorPacket.h"
#include <json.hpp>
#include <algorithm>
#include <charconv>
#include <limits>
#include <set>

namespace
{
constexpr std::size_t JsonOffset = 5;
constexpr std::size_t MaximumPacketSize = 16384;
constexpr int MaximumBaseStat = UINT16_MAX;
bool IsToken(const std::string& value, bool optional = false)
{
    return (optional && value.empty()) ||
           (value.size() == 32 && std::all_of(value.begin(), value.end(), [](char c)
                                                           { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); }));
}
bool ValidText(const std::string& value, std::size_t maximum)
{
    return value.size() <= maximum && value.find('\0') == std::string::npos;
}
} // namespace

namespace Network
{
bool ReadStatAdvisorAmount(const std::string& text, std::uint32_t& amount)
{
    amount = 0;
    if (text.empty())
        return true;
    if (text.size() > 5 || !std::all_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; }))
        return false;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), amount);
    return parsed.ec == std::errc{} && amount <= MaximumBaseStat;
}

std::optional<std::array<std::uint8_t, 91>> MakeStatAdvisorRequest(
    std::uint8_t operation, const std::string& sessionId, const std::string& token,
    const std::array<std::string, 5>& amounts)
{
    if (operation > 6 || !IsToken(sessionId, operation == 0) || !IsToken(token, true))
        return std::nullopt;
    if ((operation == 2 || operation == 5) && token.empty())
        return std::nullopt;
    std::array<std::uint8_t, 91> bytes{};
    bytes[0] = 0xC1;
    bytes[1] = static_cast<std::uint8_t>(bytes.size());
    bytes[2] = 0xF2;
    bytes[3] = 0x03;
    bytes[4] = operation;
    std::copy(sessionId.begin(), sessionId.end(), bytes.begin() + 5);
    std::copy(token.begin(), token.end(), bytes.begin() + 38);
    for (std::size_t i = 0; i < amounts.size(); ++i)
    {
        std::uint32_t value;
        if (!ReadStatAdvisorAmount(amounts[i], value))
            return std::nullopt;
        for (int shift = 0; shift < 4; ++shift)
            bytes[71 + 4 * i + shift] = static_cast<std::uint8_t>(value >> (shift * 8));
    }
    return bytes;
}

std::optional<StatAdvisorPacket> StatAdvisorPacket::Parse(std::span<const std::uint8_t> bytes)
{
    if (bytes.size() <= JsonOffset || bytes.size() > MaximumPacketSize || bytes[0] != 0xC2 || bytes[3] != 0xF2 ||
        bytes[4] != 0x03 || ((static_cast<std::size_t>(bytes[1]) << 8) | bytes[2]) != bytes.size())
        return std::nullopt;
    const auto end = std::find(bytes.begin() + JsonOffset, bytes.end(), 0);
    if (end == bytes.end())
        return std::nullopt;
    const auto json = nlohmann::json::parse(bytes.begin() + JsonOffset, end, nullptr, false);
    if (json.is_discarded() || !json.is_object())
        return std::nullopt;
    try
    {
        StatAdvisorPacket result;
        result.sessionId = json.at("SessionId").get<std::string>();
        result.token = json.at("Token").get<std::string>();
        result.title = json.at("Title").get<std::string>();
        result.message = json.at("Message").get<std::string>();
        if (!json.at("Page").is_number_integer() || !json.at("AvailablePoints").is_number_integer() ||
            !json.at("CanConfirm").is_boolean() || !json.at("ClearInputs").is_boolean())
            return std::nullopt;
        const auto page = json.at("Page").get<std::int64_t>();
        const auto points = json.at("AvailablePoints").get<std::int64_t>();
        if (page < 0 || page > 4 || points < 0 || points > std::numeric_limits<int>::max())
            return std::nullopt;
        result.page = static_cast<int>(page);
        result.available = static_cast<int>(points);
        result.canConfirm = json.at("CanConfirm").get<bool>();
        result.clearInputs = json.at("ClearInputs").get<bool>();
        if (!IsToken(result.sessionId) || !IsToken(result.token, true) ||
            result.canConfirm != !result.token.empty() || !ValidText(result.title, 192) || !ValidText(result.message, 4096))
            return std::nullopt;
        const auto& rows = json.at("Stats");
        if (!rows.is_array() || rows.size() < 4 || rows.size() > 5)
            return std::nullopt;
        std::set<int> ids;
        for (const auto& row : rows)
        {
            for (const auto* field : {"Id", "Current", "Maximum", "Amount"})
                if (!row.at(field).is_number_integer() || row.at(field).get<std::int64_t>() < 0 ||
                    row.at(field).get<std::int64_t>() > MaximumBaseStat)
                    return std::nullopt;
            StatAdvisorRow value{row.at("Id").get<int>(), row.at("Current").get<int>(), row.at("Maximum").get<int>(),
                                 row.at("Amount").get<int>(), row.at("Name").get<std::string>()};
            if (value.id > 4 || !ids.insert(value.id).second || !ValidText(value.name, 96))
                return std::nullopt;
            result.rows.push_back(std::move(value));
        }
        for (int id : {0, 1, 2, 3})
            if (!ids.contains(id))
                return std::nullopt;
        for (const auto* key : {"Reset", "Allocate", "Leave", "Back", "Clear", "Next", "ConfirmReset", "ConfirmAllocation",
                                "Attribute", "Current", "Increase", "Result", "Available", "Total", "Remaining",
                                "InvalidInput", "Waiting"})
        {
            const auto value = json.at("Labels").at(key).get<std::string>();
            if (value.empty() || !ValidText(value, 256))
                return std::nullopt;
            result.labels.emplace(key, value);
        }
        return result;
    }
    catch (const nlohmann::json::exception&)
    {
        return std::nullopt;
    }
}
} // namespace Network
