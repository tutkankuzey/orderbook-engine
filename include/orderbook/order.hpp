#pragma once

#include <cstdint>

namespace orderbook {

// Prices are integer ticks, not floating point: exact comparison
// matters when deciding whether two orders cross.
using Price    = std::int64_t;
using Quantity = std::uint64_t;
using OrderId  = std::uint64_t;

enum class Side {
    Buy,
    Sell
};

struct Trade {
    OrderId  aggressor_id;   // the incoming order
    OrderId  resting_id;     // the one already in the book
    Price    price;          // the RESTING order's price
    Quantity quantity;
};

struct Order {
    OrderId  id;
    Side     side;
    Price    price;
    Quantity quantity;

    // Declared here, defined in order.cpp.
    bool is_buy() const;
};

}