#include "Network/Server/ResetNpcDialogPacket.h"
#include "doctest.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <vector>

namespace
{
constexpr std::size_t TextOffset = 326;

std::vector<std::uint8_t> MakeDialog(std::string_view token = "")
{
    constexpr std::string_view Text = "下一次转生需要400级\n需要金币100";
    std::vector<std::uint8_t> bytes(TextOffset + Text.size() + 1);
    bytes[0] = 0xC2;
    bytes[1] = static_cast<std::uint8_t>(bytes.size() >> 8);
    bytes[2] = static_cast<std::uint8_t>(bytes.size());
    bytes[3] = 0xF2;
    bytes[4] = 1;
    std::copy(token.begin(), token.end(), bytes.begin() + 5);
    std::copy(Text.begin(), Text.end(), bytes.begin() + TextOffset);
    return bytes;
}
} // namespace

TEST_CASE("Reset NPC menu preserves UTF-8 requirements and carries no confirmation [network][reset]")
{
    const auto dialog = Network::ResetNpcDialogPacket::Parse(MakeDialog());
    REQUIRE(dialog);
    CHECK(dialog->confirmationToken.empty());
    CHECK(dialog->text == "下一次转生需要400级\n需要金币100");
}

TEST_CASE("Reset NPC confirmation accepts only a bounded hexadecimal token [network][reset]")
{
    constexpr std::string_view Token = "1234567890abcdef1234567890abcdef";
    const auto dialog = Network::ResetNpcDialogPacket::Parse(MakeDialog(Token));
    REQUIRE(dialog);
    CHECK(dialog->confirmationToken == Token);
    CHECK_FALSE(Network::ResetNpcDialogPacket::Parse(MakeDialog("/reset")));
}

TEST_CASE("Reset NPC parser rejects truncated or mismatched packets [network][reset]")
{
    auto bytes = MakeDialog();
    for (std::size_t length = 0; length < bytes.size(); ++length)
        CHECK_FALSE(Network::ResetNpcDialogPacket::Parse(std::span(bytes).first(length)));
    bytes[4] = 0;
    CHECK_FALSE(Network::ResetNpcDialogPacket::Parse(bytes));
}

TEST_CASE("Reset NPC parser requires terminated text fields [network][reset]")
{
    auto bytes = MakeDialog();
    bytes.back() = 1;
    CHECK_FALSE(Network::ResetNpcDialogPacket::Parse(bytes));
    bytes = MakeDialog();
    std::fill(bytes.begin() + 38, bytes.begin() + 134, 1);
    CHECK_FALSE(Network::ResetNpcDialogPacket::Parse(bytes));
}
