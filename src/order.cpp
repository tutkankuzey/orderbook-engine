#include <orderbook/order.hpp>

namespace orderbook {

bool Order::is_buy() const {
    return side == Side::Buy;
}

}