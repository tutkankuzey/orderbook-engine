#include <orderbook/order.hpp>
#include <orderbook/book.hpp>
#include <orderbook/price_level.hpp>

#include <algorithm>

namespace orderbook{

    std::vector<Trade> Book::add_limit_order(const Order& order){
        std::vector<Trade> trades;
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
                Quantity filled = level.fill(total, order.id, trades);
                
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
                Quantity filled = level.fill(total, order.id, trades);
                
                remaining -= filled;

                if (level.empty()){
                    bids_.erase(it);
                }
            }
        }
        return trades;
    }
}  // namespace orderbook