#include <orderbook/replay.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

using namespace orderbook;

namespace {

std::vector<Command> generate(std::size_t n, std::uint64_t seed) {

    std::mt19937_64 rng(seed);
    std::discrete_distribution<int> action_dist({50, 40, 10});  // limit/cancel/market weights
    std::uniform_int_distribution<Price> price_dist(9900, 10100);
    std::uniform_int_distribution<Quantity> qty_dist(1, 100);
    std::bernoulli_distribution side_dist(0.5);
   
    std::vector<OrderId> live_ids;
    std::size_t next_id = 1;

    constexpr int kLimit = 0;
    constexpr int kCancel = 1;
    constexpr int kMarket = 2;

    std::vector<Command> out;
    out.reserve(n);

    for (size_t i = 0; i < n; ++i){

        switch (action_dist(rng)){
            case (kCancel):{
                if (!live_ids.empty()){
                    std::uniform_int_distribution<std::size_t> pick(0, live_ids.size() - 1);
                    Command cmd = {Command::Action::Cancel,
                        live_ids[pick(rng)],
                        Side::Buy, 0,
                        0};
                    out.push_back(cmd);
                }
                break;
            }
            case (kLimit):{
                Command cmd = {Command::Action::Limit,
                    next_id, 
                    side_dist(rng) ? Side::Buy : Side::Sell, 
                    price_dist(rng),
                    qty_dist(rng)};
                out.push_back(cmd);
                live_ids.push_back(next_id);
                ++next_id;
                break;
            }
            case (kMarket):{
                Command cmd = {Command::Action::Market,
                    next_id++, 
                    side_dist(rng) ? Side::Buy : Side::Sell, 
                    0,
                    qty_dist(rng)};
                out.push_back(cmd);
                break;
            }
        }
    }
    return out;
    }
}  // anonymous namespace

int main() {
    using clock = std::chrono::steady_clock;

    constexpr std::size_t N = 1'000'000;

    auto commands = generate(N, 42);  // NOT timed

    Book book;
    std::size_t total_trades = 0;

    auto start = clock::now();
    for (const Command& cmd : commands) {
        total_trades += apply(book, cmd).size();
    }
    auto end = clock::now();
    auto duration = end - start;

    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    double seconds = microseconds / 1'000'000.0;
    double per_sec = commands.size() / seconds;

    double ns_per_order = (microseconds * 1000.0) / commands.size();

    std::cout << "Time Elapsed = " << seconds << " s\n"
            << "Orders/sec = " << per_sec << "\n"
            << "Total trades = " << total_trades << "\n"
            << "Final book size = " << book.order_count() << "\n"
            << "Nanoseconds per order = " << ns_per_order << "\n";
}