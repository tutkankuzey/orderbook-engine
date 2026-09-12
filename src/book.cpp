#include <optional>
#include <orderbook/order.hpp>
#include <orderbook/book.hpp>
#include <orderbook/price_level.hpp>

#include <algorithm>
#include <vector>

namespace orderbook{

    std::size_t Book::index_size() const { return index_.size(); }
    std::size_t Book::order_count() const {
        std::size_t count = 0;
        for (const auto& [price, level] : bids_) {
            count += level.order_count();
        }
        for (const auto& [price, level] : asks_) { 
            count += level.order_count();
        }
        return count;
    }
    std::optional<Price> Book::best_bid() const{
        if (bids_.empty()){
            return std::nullopt;
        }
        return bids_.begin()->first;
    }
    std::optional<Price> Book::best_ask() const{
        if (asks_.empty()){
            return std::nullopt;
        }
        return asks_.begin()->first;
    }

    bool Book::empty() const{
        return bids_.empty() && asks_.empty();
    }

    Quantity Book::quantity_at(Side side, Price price) const{
        if (side == Side::Buy){
            auto it = bids_.find(price);
            return it != bids_.end() ? it->second.total_quantity() : 0;
        }

        auto it = asks_.find(price);
        return it != asks_.end() ? it->second.total_quantity() : 0;
    }

    std::vector<Trade> Book::add_limit_order(const Order& order){
        std::vector<Trade> trades;
        std::vector<OrderId> exhausted;
        Quantity remaining = order.quantity;

        if (order.side == Side::Buy){
            while (remaining > 0 && !asks_.empty()){
                auto it = asks_.begin();
                Price level_price = it->first;
                PriceLevel& level = it->second;
                
                if (order.price < level_price){
                    break;
                }

                Quantity total = std::min(remaining, level.total_quantity());
                Quantity filled = level.fill(total, order.id, trades, exhausted);
                
                remaining -= filled;

                if (level.empty()){
                    asks_.erase(it);
                }
            }
        }
        else{
            while (remaining > 0 && !bids_.empty()){
                auto it = bids_.begin();
                Price level_price = it->first;
                PriceLevel& level = it->second;
                
                if (order.price > level_price){
                    break;
                }

                Quantity total = std::min(remaining, level.total_quantity());
                Quantity filled = level.fill(total, order.id, trades, exhausted);
                
                remaining -= filled;

                if (level.empty()){
                    bids_.erase(it);
                }
            }
        }
        
        for (OrderId gone : exhausted) index_.erase(gone);

        if (remaining > 0){
            if (order.side == Side::Buy){
                auto [it, bl] = bids_.try_emplace(order.price, order.price);
                it->second.add(Order{order.id, order.side, order.price, remaining});
            }
            else{
                auto [it, bl] = asks_.try_emplace(order.price, order.price);
                it->second.add(Order{order.id, order.side, order.price, remaining});
            }
            index_[order.id] = {order.side, order.price};
        }

        return trades;
    }

    std::vector<Trade> Book::add_market_order(OrderId id, Side side, Quantity quantity){
        std::vector<Trade> trades;
        std::vector<OrderId> exhausted;
        if (side == Side::Buy){
            while (quantity > 0 && !asks_.empty()){
                auto it = asks_.begin();
                PriceLevel& level = it->second;
                Quantity filled = level.fill(quantity, id, trades, exhausted);
                quantity -= filled;
                if (level.empty()) asks_.erase(it);
            }
        }
        else{
            while (quantity > 0 && !bids_.empty()){
                auto it = bids_.begin();
                PriceLevel& level = it->second;
                Quantity filled = level.fill(quantity, id, trades, exhausted);
                quantity -= filled;
                if (level.empty()) bids_.erase(it);
            }
        }

         for (OrderId gone : exhausted) index_.erase(gone);

        return trades;
    }

    bool Book::cancel(OrderId id) {
        auto idx_it = index_.find(id);
        if (idx_it == index_.end()) return false;

        const OrderLocation loc = idx_it->second;

        if (loc.side == Side::Buy) {
            auto it = bids_.find(loc.price);
            if (it == bids_.end()) return false;    // shouldn't happen; index/book disagree

            if (!it->second.remove(id)) return false;
            if (it->second.empty()) bids_.erase(it);
        } else {
            auto it = asks_.find(loc.price);
            if (it == asks_.end()) return false;

            if (!it->second.remove(id)) return false;
            if (it->second.empty()) asks_.erase(it);
        }

        index_.erase(idx_it);
        return true;
    }
}  // namespace orderbook