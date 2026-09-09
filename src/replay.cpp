#include <optional>
#include <string>
#include <sstream>

#include <orderbook/order.hpp>
#include <orderbook/book.hpp>
#include <orderbook/replay.hpp>

namespace orderbook {
namespace { // anonymous namespace
    std::optional<std::uint64_t> to_u64(const std::string& s) {
        try {
            return std::stoull(s);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
    std::optional<std::int64_t> to_i64(const std::string& s) {
        try {
            return std::stoll(s);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
}

    std::optional<Command> parse_line(const std::string &line){
        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }

        if (fields.empty()) return std::nullopt;

        if (fields[0] == "CANCEL"){
            if (fields.size() != 4) return std::nullopt;

            if (fields[2] != "" || fields[3] != "") return std::nullopt;
            auto id = to_u64(fields[1]);
            if (!id) return std::nullopt;
            return Command{Command::Action::Cancel, *id, Side::Buy, 0, 0};
        }
        else if (fields[0] == "LIMIT"){
            if (fields.size() != 5) return std::nullopt;

            auto id = to_u64(fields[1]);
            if (!id) return std::nullopt;

            if (fields[2] != "BUY" && fields[2] != "SELL") return std::nullopt;

            auto price = to_i64(fields[3]);
            if (!price) return std::nullopt;

            auto quantity = to_u64(fields[4]);
            if (!quantity) return std::nullopt;

            Side side = fields[2] == "BUY" ? Side::Buy : Side::Sell;
            return Command{Command::Action::Limit, *id, side, *price, *quantity};
        }
        else if (fields[0] == "MARKET"){
            if (fields.size() != 5) return std::nullopt;
            
            auto id = to_u64(fields[1]);
            if (!id) return std::nullopt;

            if (fields[2] != "BUY" && fields[2] != "SELL") return std::nullopt;

            // market orders dont have specific prices
            if (fields[3] != "") return std::nullopt; 

            auto quantity = to_u64(fields[4]);
            if (!quantity) return std::nullopt;
            
            Side side = fields[2] == "BUY" ? Side::Buy : Side::Sell;
            return Command{Command::Action::Market, *id, side, 0, *quantity};
        }
        
        return std::nullopt;
    }


}