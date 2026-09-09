#pragma once

#include <cstddef>
#include <optional>
#include <vector>
#include <string>

#include <orderbook/order.hpp>
#include <orderbook/book.hpp>

namespace orderbook {

    struct Command {
        enum class Action{
            Limit,
            Market,
            Cancel
        };
        Action action;
        OrderId id;
        Side side;
        Price price;
        Quantity quantity;
    };
    // Parse one line. Returns nullpt if the line doesn't match format.
    std::optional<Command> parse_line(const std::string& line);

    std::vector<Trade> apply(Book& book, const Command& cmd);
}
