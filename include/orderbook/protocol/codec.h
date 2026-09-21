#pragma once

#include "messages.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>

namespace orderbook::protocol {

// The wire format is little-endian and the message structs are packed to match
// it byte for byte (see messages.h), so encoding and decoding are copies rather
// than field-by-field conversions. That is only valid on a little-endian host.
static_assert(std::endian::native == std::endian::little,
              "Protocol v1 assumes a little-endian host; see docs/PROTOCOL.md");

// Result of attempting to decode one inbound message from a byte buffer.
//
// The gateway's response differs per status, which is why these are four
// distinct values rather than a success/failure pair:
//
//   Ok           -> hand `message` to the engine; advance the buffer by
//                   `bytes_consumed`.
//   Incomplete   -> wait for more bytes. Nothing was consumed.
//   InvalidField -> send a Reject carrying `client_order_id` and `reason`,
//                   then advance the buffer by `bytes_consumed`. The stream is
//                   still synchronised, so the connection survives.
//   Unparseable  -> the type byte is unknown, so the message length is unknown
//                   and no later byte can be trusted. Close the connection.
//                   (docs/PROTOCOL.md, tier 3.)
struct DecodeResult {
    enum class Status : std::uint8_t {
        Ok,
        Incomplete,
        InvalidField,
        Unparseable,
    };

    Status status = Status::Incomplete;

    // Ok           -> the full length of the decoded message.
    // InvalidField -> the full length of the offending message; the caller must
    //                 still skip it to stay synchronised.
    // Incomplete   -> 0.
    // Unparseable  -> 0; the caller is disconnecting, nothing is consumable.
    std::size_t bytes_consumed = 0;

    // Meaningful only when status == InvalidField. Echoed in the Reject so the
    // client knows which of its orders was refused. Always readable there:
    // InvalidField implies the message arrived in full, and the client order ID
    // sits at a fixed offset in every inbound message.
    std::uint32_t client_order_id = 0;
    RejectReason  reason          = RejectReason::InvalidSide;

    // Holds monostate unless status == Ok. std::variant is never empty, so
    // without monostate the non-Ok states would silently hold a
    // default-constructed MarketOrderMessage.
    //
    // Outbound (exchange -> client) messages are absent by design: this decoder
    // runs on the exchange side and must reject anything travelling the wrong
    // way.
    std::variant<std::monostate,
                 LimitOrderMessage,
                 MarketOrderMessage,
                 CancelMessage> message;
};

// Attempts to decode one message from the front of `buffer`.
//
// Does not own or modify the buffer: the gateway owns per-connection byte
// accumulation and removes `bytes_consumed` bytes itself. Stateless, so it is
// testable without a socket.
[[nodiscard]] DecodeResult decode(std::span<const std::uint8_t> buffer);

// Writes `msg` into `out`; returns the number of bytes written, or 0 if `out`
// is too small. Never allocates.
//
// Inbound types are encodable too: the exchange never sends them, but clients
// do, and the round-trip tests need both directions.
[[nodiscard]] std::size_t encode(const LimitOrderMessage&  msg, std::span<std::uint8_t> out);
[[nodiscard]] std::size_t encode(const MarketOrderMessage& msg, std::span<std::uint8_t> out);
[[nodiscard]] std::size_t encode(const CancelMessage&      msg, std::span<std::uint8_t> out);
[[nodiscard]] std::size_t encode(const AckMessage&         msg, std::span<std::uint8_t> out);
[[nodiscard]] std::size_t encode(const CancelledMessage& msg, std::span<std::uint8_t> out);
[[nodiscard]] std::size_t encode(const RejectMessage&      msg, std::span<std::uint8_t> out);
[[nodiscard]] std::size_t encode(const FillMessage&        msg, std::span<std::uint8_t> out);

}  // namespace orderbook::protocol