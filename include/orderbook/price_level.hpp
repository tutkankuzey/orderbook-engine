#pragma once

#include <cstddef>
#include <deque>

#include <orderbook/order.hpp>

namespace orderbook {

// All resting orders at a single price, on a single side.
// Orders are kept in arrival order: price-time priority means the
// front of the queue is filled first.
class PriceLevel {
public:
    explicit PriceLevel(Price price);

    Price price() const;

    // Total unfilled quantity across every order at this level.
    Quantity total_quantity() const;

    std::size_t order_count() const;
    bool empty() const;

    // Add an order to the back of the queue.
    void add(const Order& order);

    // Consume up to `quantity` from the front of the queue, in
    // arrival order. Returns how much was actually filled, which
    // is less than requested if the level runs dry.
    Quantity fill(Quantity quantity);

private:
    Price price_;
    Quantity total_quantity_;
    std::deque<Order> orders_;
};

}  // namespace orderbook