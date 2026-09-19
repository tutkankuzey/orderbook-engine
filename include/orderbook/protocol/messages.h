#pragma once

// Wire Layouts. pragma pack(push, 1) removes extra padding so that sizeof is equal to the lenghts specified
// in docs/PROTOCOL.md. The static assertions below the struct definitions assert just that. 

#include <cstdint>
#include <cstddef>

#pragma pack(push, 1) 
namespace orderbook::protocol {

enum class MessageType : std::uint8_t {
    LimitOrder  = 1,
    MarketOrder = 2,
    Cancel      = 3,
    Ack         = 101,
    Reject      = 102,
    Fill        = 103,
};

enum class RejectReason : std::uint8_t {
    InvalidSide        = 1,
    QtyOutOfRange      = 2,
    PriceOutOfRange    = 3,
    DuplicateClientID  = 4,
    UnknownClientID    = 5,
    NoLiquidity        = 6
};

struct LimitOrderMessage {
    std::uint8_t  type;
    std::uint8_t  side;
    std::uint32_t client_order_id;
    std::uint32_t quantity;
    std::int32_t  price;
};

struct MarketOrderMessage {
    std::uint8_t type;
    std::uint8_t side;
    std::uint32_t client_order_id;
    std::uint32_t quantity;
};

struct CancelMessage {
    std::uint8_t type;
    std::uint32_t client_order_id;
};

struct AckMessage {
    std::uint8_t type;
    std::uint32_t client_order_id;
    std::uint64_t exchange_order_id;
};

struct RejectMessage {
    std::uint8_t type;
    std::uint32_t client_order_id;
    std::uint8_t reason_code;
};

struct FillMessage {
    std::uint8_t type;
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
static_assert(sizeof(RejectMessage) == 6);
static_assert(sizeof(FillMessage) == 22);

#pragma pack(pop)
}  // namespace orderbook::protocol