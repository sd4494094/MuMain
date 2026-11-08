#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Network
{
struct StatAdvisorRow
{
    int id = 0, current = 0, maximum = 0, amount = 0;
    std::string name;
};
struct StatAdvisorPacket
{
    std::string sessionId, token, title, message;
    int page = 0, available = 0;
    bool canConfirm = false, clearInputs = false;
    std::vector<StatAdvisorRow> rows;
    std::map<std::string, std::string> labels;
    static std::optional<StatAdvisorPacket> Parse(std::span<const std::uint8_t> bytes);
};
// Stable layout is defined in OpenMU ClientToServerPackets.xml / StatAdvisorRequestExtended.
std::optional<std::array<std::uint8_t, 91>> MakeStatAdvisorRequest(
    std::uint8_t operation, const std::string& sessionId, const std::string& token,
    const std::array<std::string, 5>& amounts);
bool ReadStatAdvisorAmount(const std::string& text, std::uint32_t& amount);
} // namespace Network
