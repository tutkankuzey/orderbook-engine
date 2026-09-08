# orderbook-engine

A limit order book matching engine in C++.

## What it does

A matching engine sits at the core of an exchange. It receives orders from
participants and decides whether each one executes immediately against orders
already in the book, or rests and waits. Orders that rest are stored by side
(buy/sell) and by price level.

The engine uses **price-time priority**. For bids, a higher price is ahead of a
lower one. If two orders are at the same price, the one that arrived first is
ahead. Asks work the same way with the ordering reversed — a lower price is
better.

## Design decisions

**Prices are integer ticks, not doubles.** A price of $100.50 is stored as
`10050`. Floating point can't represent most decimal values exactly, so
`0.1 + 0.2 != 0.3`. An order book compares prices constantly to decide whether
two orders cross, and those comparisons have to be exact.

**Bids and asks are `std::map`s with opposite orderings.** The best bid is the
highest price and the best ask is the lowest, so the bid map uses
`std::greater` to reverse the default ascending order. This way `.begin()`
gives the best price on both sides, and the matching logic is symmetric.

**Each price level holds a `std::deque`.** Orders are appended at the back and
consumed from the front, both O(1), which is exactly the access pattern
price-time priority needs. The tradeoff is cancellation: removing an order from
the middle is currently O(n). Cancels are frequent in real markets, so this is
the first thing slated for optimization.

**Trades are priced at the resting order's price.** If a sell rests at $100.50
and a buy arrives at $101.00, the trade happens at $100.50. If a buy rests at
$101.00 and a sell arrives at $100.50, the trade happens at $101.00. The order
that was already there sets the price, and the incoming order gets the improvement (if any).

## Building

```bash
cmake -B build
cmake --build build
```

Requires CMake 3.16+ and a C++20 compiler. Catch2 is fetched automatically
during configuration.

## Testing

The project uses [Catch2](https://github.com/catchorg/Catch2). All test files
are in `tests/`.

```bash
cmake --build build
./build/tests
```

## Status

Working:

- Limit orders, with matching, partial fills, and multi-level sweeps
- Order cancellation
- Trade reporting
- Market orders

Not yet implemented:

- CSV replay driver
- Benchmarks

Known limitations: single instrument, single-threaded, and cancellation is
linear in the number of orders at a price level.