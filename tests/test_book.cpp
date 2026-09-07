#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <optional>
#include <orderbook/book.hpp>
#include <orderbook/order.hpp>

using namespace orderbook;

TEST_CASE("Resting in an empty book", "[book]"){
    Book book;
    Order buy{1, Side::Buy, 10010, 100};
    auto trades = book.add_limit_order(buy);
    
    REQUIRE(trades.empty());
    REQUIRE(book.quantity_at(Side::Buy, 10010) == 100);
    REQUIRE(book.best_bid() == 10010);
    REQUIRE(book.best_ask() == std::nullopt);
}

TEST_CASE("One buy and one sell resting", "[book]"){
    Book book;
    Order buy{1, Side::Buy, 10010, 100};
    Order sell{2, Side::Sell, 10050, 40};
    book.add_limit_order(buy);
    auto trades = book.add_limit_order(sell);
    
    REQUIRE(trades.empty());
    REQUIRE(!book.empty());
    REQUIRE(book.best_bid() == 10010);
    REQUIRE(book.best_ask() == 10050);
    REQUIRE(book.quantity_at(Side::Buy, 10010) == 100);
    REQUIRE(book.quantity_at(Side::Sell, 10050) == 40);
}

TEST_CASE("An aggressive sell fills at the resting bid price", "[book]"){
    Book book;
    Order buy{1, Side::Buy, 10010, 100};
    Order sell{2, Side::Sell, 10005, 40};
    book.add_limit_order(buy);
    auto trades = book.add_limit_order(sell);
    
    REQUIRE(trades.size() == 1);

    SECTION("Trades occur at the resting price") {
        auto it = trades.begin();
        REQUIRE(it->price == 10010);
        REQUIRE(it->quantity == 40);
        REQUIRE(it->aggressor_id == 2);
        REQUIRE(it->resting_id == 1);
    }

    SECTION("The remainder rests on the bid") {
        REQUIRE(book.quantity_at(Side::Buy, 10010) == 60);
        REQUIRE(book.best_ask() == std::nullopt);
    }
}

TEST_CASE("A sweep hits both levels", "[book]") {
    Book book;
    Order a{1, Side::Sell, 10005, 40};
    Order b{2, Side::Sell, 10008, 50};
    Order c{3, Side::Buy, 10010, 100};

    book.add_limit_order(a);
    book.add_limit_order(b);
    
    auto trades = book.add_limit_order(c);
    REQUIRE(trades.size() == 2);

    REQUIRE(trades[0].price == 10005);
    REQUIRE(trades[0].resting_id == 1);
    REQUIRE(trades[0].quantity == 40);
    REQUIRE(trades[0].aggressor_id == 3);

    REQUIRE(trades[1].price == 10008);
    REQUIRE(trades[1].resting_id == 2);
    REQUIRE(trades[1].quantity == 50);
    REQUIRE(trades[1].aggressor_id == 3);

    REQUIRE(book.quantity_at(Side::Sell, 10005) == 0);
    REQUIRE(book.quantity_at(Side::Sell, 10008) == 0);
    REQUIRE(book.quantity_at(Side::Buy, 10010) == 10);
    REQUIRE(book.best_ask() == std::nullopt);
}

TEST_CASE("An exact fill leaves the book empty", "[book]") {
    Book book;
    Order sell{1, Side::Sell, 10050, 100};
    Order buy{2, Side::Buy, 10060, 100};
    book.add_limit_order(sell);

    auto trades = book.add_limit_order(buy);
    REQUIRE(trades.size() == 1);
    REQUIRE(book.empty());
}