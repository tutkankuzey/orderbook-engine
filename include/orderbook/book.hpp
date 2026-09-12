#pragma once

#include <cstdint>
#include <cstddef>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

#include <orderbook/order.hpp>
#include <orderbook/price_level.hpp>

namespace orderbook{

struct OrderLocation{
    Side side;
    Price price;
};

class Book{
public:
    std::size_t index_size() const;
    std::size_t order_count() const;
    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;
    Quantity quantity_at(Side side, Price price) const;
    bool empty() const;

    std::vector<Trade> add_limit_order(const Order& order);
    bool cancel(OrderId id);

    std::vector<Trade> add_market_order(OrderId id, Side side, Quantity quantity);
private:
    std::map<Price, PriceLevel> asks_;
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    std::unordered_map<OrderId, OrderLocation> index_;
};

}