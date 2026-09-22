#pragma once

// Wire Layouts. pragma pack(push, 1) removes extra padding so that sizeof is equal to the lengths specified
// in docs/PROTOCOL.md. The static assertions below the struct definitions assert just that. 

#include <cstdint>
#include <cstddef>

#pragma pack(push, 1) 
namespace orderbook::protocol {

// Legal field ranges, from docs/PROTOCOL.md. The decoder enforces these;
// tests use them for boundary cases.
inline constexpr std::uint32_t min_quantity = 1;
inline constexpr std::uint32_t max_quantity = 100'000;
inline constexpr std::int32_t  min_price    = 1;
inline constexpr std::int32_t  max_price    = 100'000'000;

enum class MessageType : std::uint8_t {
    LimitOrder  = 1,
    MarketOrder = 2,
    Cancel      = 3,
    Ack         = 101,
    Reject      = 102,
    Fill        = 103,
    Cancelled   = 104,
};

enum class RejectReason : std::uint8_t {
    InvalidSide        = 1,
    QtyOutOfRange      = 2,
    PriceOutOfRange    = 3,
    DuplicateClientID  = 4,
    UnknownClientID    = 5,
    NoLiquidity        = 6
};

enum class Side : std::uint8_t{ 
    Buy = 1,
    Sell = 2
};

struct LimitOrderMessage {
    MessageType  type;
    Side side;
    std::uint32_t client_order_id;
    std::uint32_t quantity;
    std::int32_t  price;
};

struct MarketOrderMessage {
    MessageType  type;
    Side side;
    std::uint32_t client_order_id;
    std::uint32_t quantity;
};

struct CancelMessage {
    MessageType  type;
    std::uint32_t client_order_id;
};

struct AckMessage {
    MessageType  type;
    std::uint32_t client_order_id;
    std::uint64_t exchange_order_id;
};

struct CancelledMessage {
    MessageType  type;
    std::uint32_t client_order_id;
    std::uint64_t exchange_order_id;
    std::uint32_t cancelled_quantity;
};

struct RejectMessage {
    MessageType  type;
    std::uint32_t client_order_id;
    RejectReason reason_code;
};

struct FillMessage {
    MessageType  type;
    std::uint32_t client_order_id;
    std::uint64_t exchange_order_id;
    std::int32_t fill_price;
    std::uint32_t fill_quantity;
    std::uint8_t remaining;
};

static_assert(sizeof(LimitOrderMessage) == 14);
static_assert(sizeof(MarketOrderMessage) == 10);
static_assert(sizeof(CancelMessage) == 5);
static_assert(sizeof(AckMessage) == 13);
static_assert(sizeof(CancelledMessage) == 17);
static_assert(sizeof(RejectMessage) == 6);
static_assert(sizeof(FillMessage) == 22);

#pragma pack(pop)
}  // namespace orderbook::protocol