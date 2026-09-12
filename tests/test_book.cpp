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

TEST_CASE("Erasing a single order", "[book]"){
    Book book;
    Order sell{1, Side::Sell, 10050, 100};
    book.add_limit_order(sell);
    book.cancel(1);

    REQUIRE(book.empty());
    REQUIRE(book.best_ask() == std::nullopt);
}

TEST_CASE("Erasing an invalid order", "[book]"){
    Book book;
    Order sell{1, Side::Sell, 10050, 100};
    book.add_limit_order(sell);

    REQUIRE(!book.cancel(42));
    REQUIRE(!book.empty());
}

TEST_CASE("Tests with two orders", "[book]"){
    Book book;

    SECTION("Two orders at the same price, cancel one"){
        Order a{1, Side::Buy, 10010, 100};
        Order b{2, Side::Buy, 10010, 50};
        book.add_limit_order(a);
        book.add_limit_order(b);

        book.cancel(1);
        REQUIRE(book.quantity_at(Side::Buy, 10010) == 50);
    }

    SECTION("Two levels, cancel the better one"){
        Order a{1, Side::Buy, 10010, 100};
        Order b{2, Side::Buy, 10050, 50};
        book.add_limit_order(a);
        book.add_limit_order(b);

        book.cancel(2);
        REQUIRE(book.best_bid() == 10010);
    }

    SECTION("Cancelling a partially filled order decrements by the remainder") {
        Order a{1, Side::Buy, 10010, 100};
        Order b{2, Side::Buy, 10010, 25};
        Order c{3, Side::Sell, 10000, 40};
        book.add_limit_order(a);
        book.add_limit_order(b);
        book.add_limit_order(c);  // fills 40 from order 1 because order 1 came before order 2

        REQUIRE(book.quantity_at(Side::Buy, 10010) == 85);      // 60 + 25

        book.cancel(1);
        REQUIRE(book.quantity_at(Side::Buy, 10010) == 25);      // only order 2 remains
    }

}

TEST_CASE("Cancel prevents matching", "[book]"){
    Book book;
    Order a{1, Side::Buy, 10010, 100};
    book.add_limit_order(a);
    book.cancel(1);

    Order b{2, Side::Sell, 10005, 50};
    auto trades = book.add_limit_order(b);

    REQUIRE(trades.empty());
    REQUIRE(book.best_ask() == 10005);
}

TEST_CASE("A market buy fills against a resting sell", "[book]"){
    Book book;
    Order a{1, Side::Sell, 10010, 100};
    book.add_limit_order(a);
    auto trades = book.add_market_order(2, Side::Buy, 100);

    REQUIRE(book.best_ask() == std::nullopt);
    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].price == 10010);
}

TEST_CASE("Market buy sweeping two levels", "[book]"){
    Book book;
    Order a{1, Side::Sell, 10010, 100};
    Order b{2, Side::Sell, 10020, 200};
    book.add_limit_order(a);
    book.add_limit_order(b);

    SECTION("Market order doesn't empty book"){
        auto trades = book.add_market_order(3, Side::Buy, 250);

        REQUIRE(trades.size() == 2);
        REQUIRE(trades[0].price == 10010);
        REQUIRE(trades[0].quantity == 100);
        REQUIRE(trades[1].price == 10020);
        REQUIRE(trades[1].quantity == 150);
    }

    SECTION("Market order larger than the whole book"){
        auto trades = book.add_market_order(3, Side::Buy, 500);
        REQUIRE(trades.size() == 2);
        REQUIRE(book.best_ask() == std::nullopt);   // the sweep consumed everything
        REQUIRE(book.best_bid() == std::nullopt);   // the remainder was dropped, not rested
    }
}

TEST_CASE("Market order into an empty book", "[book]"){
    Book book;
    auto trades = book.add_market_order(1, Side::Buy, 100);
    REQUIRE(book.empty());
    REQUIRE(trades.empty());
}

TEST_CASE("Market sell against resting bids", "[book]"){
    Book book;
    Order a{1, Side::Buy, 10020, 100};
    Order b{2, Side::Buy, 10010, 200};
    book.add_limit_order(a);
    book.add_limit_order(b);

    auto trades = book.add_market_order(3, Side::Sell, 350);
    REQUIRE(trades.size() == 2);
    REQUIRE(trades[0].price == 10020);
    REQUIRE(trades[0].quantity == 100);
    REQUIRE(trades[1].price == 10010);
    REQUIRE(trades[1].quantity == 200);
    REQUIRE(book.empty());
}

TEST_CASE("A filled order leaves no stale index entry", "[book]") {
    Book book;
    book.add_limit_order(Order{1, Side::Buy, 10010, 100});
    book.add_limit_order(Order{2, Side::Sell, 10010, 100});   // fully fills order 1

    REQUIRE(book.empty());
    REQUIRE(book.index_size() == 0);   // needs a new accessor
}