#include "Network/Server/LevelingWarpDialogPacket.h"
#include "doctest.h"
#include <json.hpp>
#include <algorithm>
#include <string_view>
#include <vector>

namespace
{
nlohmann::json Rows()
{
    return nlohmann::json::array({{{"Number", 104},
                                   {"Name", "天空之城"},
                                   {"Description", "首领·天魔菲尼斯"},
                                   {"Level", 170},
                                   {"Costs", 10000},
                                   {"Enabled", true}}});
}
std::vector<std::uint8_t> Packet(const nlohmann::json& rows)
{
    const auto text = rows.dump();
    constexpr std::size_t offset = 262;
    std::vector<std::uint8_t> bytes(offset + text.size() + 1);
    bytes[0] = 0xC2;
    bytes[1] = static_cast<std::uint8_t>(bytes.size() >> 8);
    bytes[2] = static_cast<std::uint8_t>(bytes.size());
    bytes[3] = 0xF2;
    bytes[4] = 2;
    constexpr std::string_view token = "0123456789abcdef0123456789abcdef";
    std::copy(token.begin(), token.end(), bytes.begin() + 5);
    std::copy(text.begin(), text.end(), bytes.begin() + offset);
    return bytes;
}
} // namespace

TEST_CASE("Leveling menu preserves full Chinese names and stable destination numbers")
{
    const auto parsed = Network::LevelingWarpDialogPacket::Parse(Packet(Rows()));
    REQUIRE(parsed);
    REQUIRE(parsed->entries.size() == 1);
    CHECK(parsed->entries[0].number == 104);
    CHECK(parsed->entries[0].name == "天空之城");
    CHECK(parsed->entries[0].description == "首领·天魔菲尼斯");
    CHECK(parsed->entries[0].costs == 10000);
}

TEST_CASE("Leveling menu rejects every truncation and invalid framing")
{
    auto bytes = Packet(Rows());
    for (std::size_t n = 0; n < bytes.size(); ++n)
        CHECK_FALSE(Network::LevelingWarpDialogPacket::Parse(std::span(bytes).first(n)));
    SUBCASE("unterminated")
    {
        bytes.back() = 1;
    }
    SUBCASE("wrong opcode")
    {
        bytes[4] = 1;
    }
    SUBCASE("invalid token")
    {
        bytes[5] = '/';
    }
    CHECK_FALSE(Network::LevelingWarpDialogPacket::Parse(bytes));
}

TEST_CASE("Leveling menu rejects integer overflow, duplicate identifiers and unsafe labels")
{
    auto rows = Rows();
    SUBCASE("huge positive id")
    {
        rows[0]["Number"] = 4294967400ULL;
    }
    SUBCASE("huge negative id")
    {
        rows[0]["Number"] = -4294967192LL;
    }
    SUBCASE("huge cost")
    {
        rows[0]["Costs"] = 4294967297ULL;
    }
    SUBCASE("huge level")
    {
        rows[0]["Level"] = 4294967466ULL;
    }
    SUBCASE("negative cost")
    {
        rows[0]["Costs"] = -1;
    }
    SUBCASE("duplicate id")
    {
        rows.push_back(rows[0]);
    }
    SUBCASE("empty rows")
    {
        rows.clear();
    }
    SUBCASE("too many")
    {
        for (int n = 1; n <= 64; ++n)
        {
            auto row = rows[0];
            row["Number"] = n;
            rows.push_back(row);
        }
    }
    SUBCASE("null name")
    {
        rows[0]["Name"] = std::string("abc\0def", 7);
    }
    SUBCASE("newline description")
    {
        rows[0]["Description"] = "abc\ndef";
    }
    SUBCASE("long name")
    {
        rows[0]["Name"] = std::string(97, 'x');
    }
    SUBCASE("missing field")
    {
        rows[0].erase("Enabled");
    }
    CHECK_FALSE(Network::LevelingWarpDialogPacket::Parse(Packet(rows)));
}
