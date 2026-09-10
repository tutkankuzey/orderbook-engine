#include <orderbook/replay.hpp>

#include <fstream>
#include <iostream>
#include <string>

bool is_blank(const std::string& line) {
    // Looks for anything that is NOT a space, tab, or carriage return (\r)
    return line.find_first_not_of(" \t\r\n") == std::string::npos;
}

int main(int argc, char* argv[]) {
    using namespace orderbook;

    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <orders.csv>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "could not open " << argv[1] << "\n";
        return 1;
    }

    Book book;
    std::string line;
    int line_number = 0;
    int trade_count = 0;

    while (std::getline(file, line)) {
        ++line_number;

        if (is_blank(line) || line[0] == '#') continue;


        auto cmd = parse_line(line);
        if (!cmd) {
            std::cerr << " error at line " << line_number << "\n";
            continue;
        }

        auto trades = apply(book, *cmd);

        for (const Trade& trade : trades){
            std::cout << "TRADE  aggressor=" << trade.aggressor_id
            << "  resting=" << trade.resting_id
            << "  price=" << trade.price
            << "  qty=" << trade.quantity << "\n";
            trade_count ++;
        }
    }

    std::cout << "Total trades: " << trade_count << "\n";
    if (auto bid = book.best_bid()){
        std::cout << "Best bid: " << *bid << "\n";
    }
    else std::cout << "Best bid: none\n";
    
    if (auto ask = book.best_ask()){
        std::cout << "Best ask: " << *ask << "\n";
    }
    else std::cout << "Best ask: none\n";

    return 0;
}