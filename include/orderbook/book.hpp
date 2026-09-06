#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

#include <orderbook/order.hpp>
#include <orderbook/price_level.hpp>

namespace orderbook{

class Book{
public:
    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;
    Quantity quantity_at(Side side, Price price) const;
    bool empty() const;


    std::vector<Trade> add_limit_order(const Order& order);
private:
    std::map<Price, PriceLevel> asks_;
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
};

}