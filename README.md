# orderbook-engine
[![CI](https://github.com/tutkankuzey/orderbook-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/tutkankuzey/orderbook-engine/actions/workflows/ci.yml)

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
price-time priority needs. Removing an order from the middle is still linear in
the level's size, so cancellation is O(orders at that price) rather than truly
constant — see Performance below.

**Trades are priced at the resting order's price.** If a sell rests at $100.50
and a buy arrives at $101.00, the trade happens at $100.50. If a buy rests at
$101.00 and a sell arrives at $100.50, the trade happens at $101.00. The order
that was already there sets the price, and the incoming order gets the improvement (if any).

**Orders are indexed by ID.** An `unordered_map<OrderId, {side, price}>` maps
each resting order to its location. Without it, cancelling means scanning every
level on both sides. The cost is keeping the index in sync: `PriceLevel::fill`
reports which orders it exhausted so the book can remove them.

## Performance

1,000,000 commands (50% limit, 40% cancel, 10% market), M-series MacBook Air,
`RelWithDebInfo`. The numbers written are the medians obtained after three runs. The benchmark is
deterministic (the trade count and final book size are the same every run), so the spread
between runs is machine noise.


| Version                                | Orders/sec | ns/order |
| -------------------------------------- | ---------- | -------- |
| Baseline (`std::map` scan for cancels)* | 370,000    | 2,700    |
| With order-ID index                     | 9,930,000  | 101      |

\* Measured on the pre-index version; not reproducible from the current tree.

Reproduce with:

```bash
cmake -B build --fresh -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/bench
```

`-O3` (`CMAKE_BUILD_TYPE=Release`) gives no measurable improvement over the
`-O2` used here: the hot path is bound by memory access rather than
arithmetic.

The baseline cancel path scanned every price level on both sides, and every
order within each level. Indexing order IDs to their (side, price) location
reduces that to a single hash lookup plus one map lookup.

## Building and Running

```bash
cmake -B build
cmake --build build
./build/engine data/example.csv
```
Requires CMake 3.16+ and a C++20 compiler. Tested with Apple Clang 17 (macOS,
arm64) and GCC on Ubuntu via CI. Catch2 is fetched automatically during
configuration.

## CSV format

The replay driver takes a path to a CSV file:

```bash
./build/engine path/to/orders.csv
```

Each line is `action,id,side,price,quantity`. Blank lines and lines
beginning with `#` are ignored.

| action | id | side | price | quantity |
|---|---|---|---|---|
| `LIMIT` | required | `BUY` or `SELL` | required | required |
| `MARKET` | required | `BUY` or `SELL` | must be empty | required |
| `CANCEL` | required | must be empty | must be empty | must be empty |

Prices are integer ticks, not decimals: `10050` means $100.50. All
keywords are uppercase.

Invalid lines are reported to stderr and skipped.

Below is an example of valid lines:

```csv
# action,id,side,price,quantity
LIMIT,1,BUY,10050,120
LIMIT,2,BUY,10100,80
LIMIT,3,SELL,10200,100
LIMIT,4,SELL,10250,140
LIMIT,5,BUY,10100,60
CANCEL,1,,,
LIMIT,6,SELL,10100,100
MARKET,7,BUY,,150
LIMIT,8,BUY,10150,50
MARKET,9,SELL,,200
```

Below is an example output:
```
TRADE  aggressor=6  resting=2  price=10100  qty=80
TRADE  aggressor=6  resting=5  price=10100  qty=20
TRADE  aggressor=7  resting=3  price=10200  qty=100
TRADE  aggressor=7  resting=4  price=10250  qty=50
TRADE  aggressor=9  resting=8  price=10150  qty=50
TRADE  aggressor=9  resting=5  price=10100  qty=40
Total trades: 6
Best bid: none
Best ask: 10250
```
## Testing

The project uses [Catch2](https://github.com/catchorg/Catch2). All test files
are in `tests/`.

```bash
cmake --build build
./build/tests
```

The engine is also checked under AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g"
cmake --build build-asan
./build-asan/tests
./build-asan/engine data/example.csv
```

Both the test suite and the CSV replay run clean.

## Status

Working:

- Limit orders, with matching, partial fills, and multi-level sweeps
- Order cancellation
- Trade reporting
- Market orders
- CSV replay driver
- Benchmarks
- Indexed cancellation (O(1) level lookup)

Known limitations: single instrument, single-threaded, cancellation is O(1) to locate a price level but linear within it

This is **v1.0**: the single-threaded matching core. The next stage extends it
into an exchange: a TCP order gateway, UDP multicast market data with sequence
numbers and gap detection, a lock-free concurrent pipeline, deterministic
journal replay, and a frequent-batch-auction matching mode for comparison
against continuous matching.