#include <catch2/catch_test_macros.hpp>
#include <orderbook/replay.hpp>

using namespace orderbook;

TEST_CASE("A limit order parses", "[replay]") {
    auto cmd = parse_line("LIMIT,1,BUY,10005,100");

    REQUIRE(cmd.has_value());
    REQUIRE(cmd->action == Command::Action::Limit);
    REQUIRE(cmd->id == 1);
    REQUIRE(cmd->side == Side::Buy);
    REQUIRE(cmd->price == 10005);
    REQUIRE(cmd->quantity == 100);
}

TEST_CASE("An unknown action is rejected", "[replay]") {
    REQUIRE_FALSE(parse_line("DELETE,1,BUY,100,50").has_value());
}

TEST_CASE("A cancel parses", "[replay]") {
    auto cmd = parse_line("CANCEL,1,,,");

    REQUIRE(cmd.has_value());
    REQUIRE(cmd->action == Command::Action::Cancel);
    REQUIRE(cmd->id == 1);
}

TEST_CASE("Malformed lines are rejected", "[replay]") {
    REQUIRE_FALSE(parse_line("LIMIT,abc,BUY,10005,100").has_value());
    REQUIRE_FALSE(parse_line("LIMIT,1,BUY,xyz,100").has_value());
    REQUIRE_FALSE(parse_line("LIMIT,1,HOLD,10005,100").has_value());
    REQUIRE_FALSE(parse_line("LIMIT,1,BUY").has_value());
    REQUIRE_FALSE(parse_line("").has_value());
    REQUIRE_FALSE(parse_line("MARKET,1,BUY,10005,100").has_value());
    REQUIRE_FALSE(parse_line("CANCEL,1,BUY,,").has_value());
}