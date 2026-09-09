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

TEST_CASE("Limit command produces resting order at the right price and quantity", "[replay]"){
    Book book;
    apply(book, *parse_line("LIMIT,1,BUY,10005,100"));
    REQUIRE(book.quantity_at(Side::Buy, 10005) == 100);
}

TEST_CASE("A market command produces trades against resting orders", "[replay]"){
    Book book;

    apply(book, *parse_line("LIMIT,1,BUY,10010,100"));
    apply(book, *parse_line("LIMIT,42,BUY,10030,100"));
    auto trades = apply(book, *parse_line("MARKET,3,SELL,,120"));
    REQUIRE(trades.size() == 2);

    REQUIRE(trades[0].aggressor_id == 3);
    REQUIRE(trades[0].resting_id == 42);
    REQUIRE(trades[0].price == 10030);

    REQUIRE(trades[1].aggressor_id == 3);
    REQUIRE(trades[1].resting_id == 1);
    REQUIRE(trades[1].price == 10010);

    REQUIRE(book.quantity_at(Side::Buy, 10030) == 0);
    REQUIRE(book.quantity_at(Side::Buy, 10010) == 80);
    REQUIRE(book.best_bid() == 10010);
}

TEST_CASE("A cancel command removes a resting order", "[replay]") {
    Book book;
    apply(book, *parse_line("LIMIT,42,BUY,10005,100"));
    apply(book, *parse_line("CANCEL,42,,,"));
    REQUIRE(book.empty());
}