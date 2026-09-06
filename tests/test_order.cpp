#include <catch2/catch_test_macros.hpp>
#include <orderbook/order.hpp>

using namespace orderbook;

TEST_CASE("Order reports its side", "[order]") {
    Order buy{1, Side::Buy, 10010, 100};
    Order sell{2, Side::Sell, 10020, 50};

    REQUIRE(buy.is_buy());
    REQUIRE_FALSE(sell.is_buy());
}

TEST_CASE("Prices are exact integers", "[order]") {
    // The point of integer ticks: this comparison is exact,
    // which it would not reliably be with double.
    Order a{1, Side::Buy,  10010, 100};
    Order b{2, Side::Sell, 10010, 100};

    REQUIRE(a.price == b.price);
}