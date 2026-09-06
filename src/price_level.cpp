#include <orderbook/price_level.hpp>

#include <algorithm>

namespace orderbook {

PriceLevel::PriceLevel(Price price)
    : price_(price), total_quantity_(0) {}

Price PriceLevel::price() const { return price_; }

Quantity PriceLevel::total_quantity() const { return total_quantity_; }

std::size_t PriceLevel::order_count() const { return orders_.size(); }

bool PriceLevel::empty() const { return orders_.empty(); }

void PriceLevel::add(const Order& order) {
    total_quantity_ += order.quantity;
    orders_.push_back(order);
}

Quantity PriceLevel::fill(Quantity quantity, OrderId aggressor, std::vector<Trade>& out){
    Quantity filled = 0;

    while (quantity > 0 && !orders_.empty()) {
        Order& front = orders_.front();

        // Take the smaller of what's wanted and what's available,
        // so neither counter can wrap around.
        const Quantity take = std::min(quantity, front.quantity);

        front.quantity  -= take;
        total_quantity_ -= take;
        quantity        -= take;
        filled          += take;

        out.push_back(Trade{aggressor, front.id, price_, take});
        if (front.quantity == 0) {
            orders_.pop_front();
        }
    }

    return filled;
}

}  // namespace orderbook