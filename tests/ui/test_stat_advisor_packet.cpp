#include "stdafx.h"
#include <doctest.h>
#include "Network/Server/StatAdvisorPacket.h"
#include <string>
#include <vector>

namespace
{
std::vector<std::uint8_t> MakeReply(const std::string& json)
{
    std::vector<std::uint8_t> packet(5 + json.size() + 1);
    packet[0] = 0xC2;
    packet[1] = static_cast<std::uint8_t>(packet.size() >> 8);
    packet[2] = static_cast<std::uint8_t>(packet.size());
    packet[3] = 0xF2;
    packet[4] = 0x03;
    std::copy(json.begin(), json.end(), packet.begin() + 5);
    return packet;
}

const char* labels = R"json({"Reset":"洗点","Allocate":"批量加点","Leave":"离开","Back":"返回","Clear":"清空","Next":"下一步","ConfirmReset":"确认洗点","ConfirmAllocation":"确认加点","Attribute":"属性","Current":"当前","Increase":"增加","Result":"结果","Available":"可用","Total":"合计","Remaining":"剩余","InvalidInput":"无效","Waiting":"等待"})json";

std::string ReplyJson(const std::string& clearInputs = "false")
{
    return std::string("{\"SessionId\":\"0123456789abcdef0123456789abcdef\",\"Token\":\"\",\"Page\":2,") +
        "\"Title\":\"洗点师·艾琳\",\"Message\":\"请输入加点数\",\"CanConfirm\":false,\"ClearInputs\":" + clearInputs +
        ",\"AvailablePoints\":7,\"Stats\":[{\"Id\":0,\"Name\":\"力量\",\"Current\":28,\"Maximum\":65535,\"Amount\":0}," +
        "{\"Id\":1,\"Name\":\"敏捷\",\"Current\":20,\"Maximum\":65535,\"Amount\":0}," +
        "{\"Id\":2,\"Name\":\"体力\",\"Current\":25,\"Maximum\":65535,\"Amount\":0}," +
        "{\"Id\":3,\"Name\":\"智力\",\"Current\":10,\"Maximum\":65535,\"Amount\":0}],\"Labels\":" + labels + "}";
}
} // namespace

TEST_CASE("stat advisor request encodes only bounded nonnegative increases")
{
    std::array<std::string, 5> amounts{"2", "", "0", "65535", "1"};
    const auto packet = Network::MakeStatAdvisorRequest(4, "0123456789abcdef0123456789abcdef", "", amounts);
    REQUIRE(packet.has_value());
    CHECK((*packet)[0] == 0xC1);
    CHECK((*packet)[1] == 91);
    CHECK((*packet)[2] == 0xF2);
    CHECK((*packet)[3] == 0x03);
    CHECK((*packet)[4] == 4);
    CHECK((*packet)[71] == 2);
    CHECK((*packet)[83] == 0xff);
    CHECK((*packet)[84] == 0xff);
    CHECK((*packet)[87] == 1);
    CHECK_FALSE(Network::MakeStatAdvisorRequest(5, "0123456789abcdef0123456789abcdef", "", amounts));
    amounts[0] = "-1";
    CHECK_FALSE(Network::MakeStatAdvisorRequest(4, "0123456789abcdef0123456789abcdef", "", amounts));
    amounts[0] = "65536";
    CHECK_FALSE(Network::MakeStatAdvisorRequest(4, "0123456789abcdef0123456789abcdef", "", amounts));
}

TEST_CASE("stat advisor response requires its complete typed bounded schema")
{
    auto bytes = MakeReply(ReplyJson("true"));
    auto parsed = Network::StatAdvisorPacket::Parse(bytes);
    REQUIRE(parsed.has_value());
    CHECK(parsed->page == 2);
    CHECK(parsed->clearInputs);
    CHECK(parsed->available == 7);
    CHECK(parsed->rows.size() == 4);
    CHECK(parsed->rows.front().name == "力量");

    bytes[4] = 0x02;
    CHECK_FALSE(Network::StatAdvisorPacket::Parse(bytes));
    bytes = MakeReply(ReplyJson("1"));
    CHECK_FALSE(Network::StatAdvisorPacket::Parse(bytes));
    bytes = MakeReply(ReplyJson());
    bytes[2]--;
    CHECK_FALSE(Network::StatAdvisorPacket::Parse(bytes));
}

TEST_CASE("stat advisor amount parser accepts blank as zero and rejects overflow or syntax")
{
    std::uint32_t amount = 99;
    CHECK(Network::ReadStatAdvisorAmount("", amount));
    CHECK(amount == 0);
    CHECK(Network::ReadStatAdvisorAmount("65535", amount));
    CHECK(amount == 65535);
    CHECK_FALSE(Network::ReadStatAdvisorAmount("1.0", amount));
    CHECK_FALSE(Network::ReadStatAdvisorAmount("65536", amount));
    CHECK_FALSE(Network::ReadStatAdvisorAmount("999999", amount));
}
