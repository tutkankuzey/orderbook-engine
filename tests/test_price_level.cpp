#include <catch2/catch_test_macros.hpp>
#include <orderbook/price_level.hpp>

using namespace orderbook;

TEST_CASE("A new level is empty", "[level]") {
    PriceLevel level(10010);

    REQUIRE(level.price() == 10010);
    REQUIRE(level.empty());
    REQUIRE(level.total_quantity() == 0);
    REQUIRE(level.order_count() == 0);
}

TEST_CASE("Adding orders accumulates quantity", "[level]") {
    PriceLevel level(10010);
    level.add(Order{1, Side::Buy, 10010, 100});
    level.add(Order{2, Side::Buy, 10010, 50});

    REQUIRE_FALSE(level.empty());
    REQUIRE(level.order_count() == 2);
    REQUIRE(level.total_quantity() == 150);
}

TEST_CASE("Fills consume orders in arrival order", "[level]") {
    std::vector<Trade> trades;
    PriceLevel level(10010);
    level.add(Order{1, Side::Buy, 10010, 100});
    level.add(Order{2, Side::Buy, 10010, 50});
    
    SECTION("a partial fill leaves the first order in place") {
        
        REQUIRE(level.fill(30, 99, trades) == 30);
        REQUIRE(level.order_count() == 2);
        REQUIRE(level.total_quantity() == 120);
    }

    SECTION("exhausting the first order removes it") {
        REQUIRE(level.fill(100, 99, trades) == 100);
        REQUIRE(level.order_count() == 1);
        REQUIRE(level.total_quantity() == 50);
    }

    SECTION("a fill can span multiple orders") {
        REQUIRE(level.fill(120, 99, trades) == 120);
        REQUIRE(level.order_count() == 1);
        REQUIRE(level.total_quantity() == 30);
    }

    SECTION("filling more than available takes only what exists") {
        REQUIRE(level.fill(500, 99, trades) == 150);
        REQUIRE(level.empty());
        REQUIRE(level.total_quantity() == 0);
    }

    SECTION("a fill spanning two orders reports two trades") {
        REQUIRE(level.fill(120, 99, trades) == 120);

        REQUIRE(trades.size() == 2);

        REQUIRE(trades[0].aggressor_id == 99);
        REQUIRE(trades[0].resting_id == 1);
        REQUIRE(trades[0].price == 10010);
        REQUIRE(trades[0].quantity == 100);

        REQUIRE(trades[1].resting_id == 2);
        REQUIRE(trades[1].quantity == 20);
    }
}

TEST_CASE("Filling an empty level is a no-op", "[level]") {
    PriceLevel level(10010);
    std::vector<Trade> trades;
    REQUIRE(level.fill(100, 99, trades) == 0);
    REQUIRE(level.empty());
}