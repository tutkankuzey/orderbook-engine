#include "orderbook/protocol/codec.h"

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>

using namespace orderbook::protocol;

// ---------------------------------------------------------------------------
// Byte-exact layout tests. Expected bytes are computed by hand from
// docs/PROTOCOL.md, NOT from the code's output. Each "expected" array has its
// length written as a literal; comparing it with a buffer of sizeof(T) fails to
// compile if the struct size ever drifts from the spec.
// ---------------------------------------------------------------------------

TEST_CASE("LimitOrderMessage matches the PROTOCOL.md worked example", "[codec][encode]") {
    // Buy 100 @ $100.50, client order id 7 -- the exact example in the spec.
    const LimitOrderMessage msg{MessageType::LimitOrder, Side::Buy, 7, 100, 10050};

    std::array<std::uint8_t, sizeof(LimitOrderMessage)> buffer{};
    REQUIRE(encode(msg, buffer) == sizeof(LimitOrderMessage));

    const std::array<std::uint8_t, 14> expected{
        0x01,                    // type  = 1 (limit)
        0x01,                    // side  = 1 (buy)
        0x07, 0x00, 0x00, 0x00,  // client order id = 7
        0x64, 0x00, 0x00, 0x00,  // quantity = 100
        0x42, 0x27, 0x00, 0x00,  // price = 10050 = 0x2742
    };
    REQUIRE(buffer == expected);
}

TEST_CASE("MarketOrderMessage encodes to the layout in PROTOCOL.md", "[codec][encode]") {
    const MarketOrderMessage msg{MessageType::MarketOrder, Side::Sell, 8, 250};

    std::array<std::uint8_t, sizeof(MarketOrderMessage)> buffer{};
    REQUIRE(encode(msg, buffer) == sizeof(MarketOrderMessage));

    const std::array<std::uint8_t, 10> expected{
        0x02,                    // type = 2 (market)
        0x02,                    // side = 2 (sell)
        0x08, 0x00, 0x00, 0x00,  // client order id = 8
        0xFA, 0x00, 0x00, 0x00,  // quantity = 250 = 0xFA
    };
    REQUIRE(buffer == expected);
}

TEST_CASE("CancelMessage encodes to the layout in PROTOCOL.md", "[codec][encode]") {
    const CancelMessage msg{MessageType::Cancel, 7};

    std::array<std::uint8_t, sizeof(CancelMessage)> buffer{};
    REQUIRE(encode(msg, buffer) == sizeof(CancelMessage));

    const std::array<std::uint8_t, 5> expected{
        0x03,                    // type = 3 (cancel)
        0x07, 0x00, 0x00, 0x00,  // client order id = 7
    };
    REQUIRE(buffer == expected);
}

TEST_CASE("AckMessage encodes to the layout in PROTOCOL.md", "[codec][encode]") {
    const AckMessage msg{MessageType::Ack, 7, 1'000'000};

    std::array<std::uint8_t, sizeof(AckMessage)> buffer{};
    REQUIRE(encode(msg, buffer) == sizeof(AckMessage));

    const std::array<std::uint8_t, 13> expected{
        0x65,                                            // type = 101
        0x07, 0x00, 0x00, 0x00,                          // client order id = 7
        0x40, 0x42, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00,  // exchange id = 1,000,000 = 0x0F4240
    };
    REQUIRE(buffer == expected);
}

TEST_CASE("RejectMessage encodes to the layout in PROTOCOL.md", "[codec][encode]") {
    const RejectMessage msg{MessageType::Reject, 7,
                            RejectReason::QtyOutOfRange};

    std::array<std::uint8_t, sizeof(RejectMessage)> buffer{};
    REQUIRE(encode(msg, buffer) == sizeof(RejectMessage));

    const std::array<std::uint8_t, 6> expected{
        0x66,                    // type = 102
        0x07, 0x00, 0x00, 0x00,  // client order id = 7
        0x02,                    // reason = 2 (quantity out of range)
    };
    REQUIRE(buffer == expected);
}

TEST_CASE("FillMessage encodes to the layout in PROTOCOL.md", "[codec][encode]") {
    // Partial fill: 30 of the order executed at $100.50, remainder still live.
    const FillMessage msg{MessageType::Fill, 7, 1'000'000, 10050, 30, 1};

    std::array<std::uint8_t, sizeof(FillMessage)> buffer{};
    REQUIRE(encode(msg, buffer) == sizeof(FillMessage));

    const std::array<std::uint8_t, 22> expected{
        0x67,                                            // type = 103
        0x07, 0x00, 0x00, 0x00,                          // client order id = 7
        0x40, 0x42, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00,  // exchange id = 1,000,000
        0x42, 0x27, 0x00, 0x00,                          // fill price = 10050
        0x1E, 0x00, 0x00, 0x00,                          // fill quantity = 30 = 0x1E
        0x01,                                            // remaining flag = 1 (partial)
    };
    REQUIRE(buffer == expected);
}

TEST_CASE("CancelledMessage encodes to the layout in PROTOCOL.md", "[codec][encode]") {
    const CancelledMessage msg{MessageType::Cancelled, 333, 555, 50};

    std::array<std::uint8_t, sizeof(CancelledMessage)> buffer{};
    REQUIRE(encode(msg, buffer) == sizeof(CancelledMessage));

    const std::array<std::uint8_t, 17> expected{
        0x68,                                            // type = 104
        0x4D, 0x01, 0x00, 0x00,                          // client order id = 333
        0x2B, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // exchange id = 555
        0x32, 0x00, 0x00, 0x00,                          // cancelled quantity = 50
    };
    REQUIRE(buffer == expected);
}

// ---------------------------------------------------------------------------
// Guard test, runs once per message type.
// ---------------------------------------------------------------------------

TEMPLATE_TEST_CASE("encode refuses a buffer one byte too small", "[codec][encode]",
                   LimitOrderMessage, MarketOrderMessage, CancelMessage,
                   AckMessage, RejectMessage, FillMessage, CancelledMessage) {
    const TestType msg{};
    std::array<std::uint8_t, sizeof(TestType) - 1> buffer{};

    REQUIRE(encode(msg, buffer) == 0);
}

TEMPLATE_TEST_CASE("encode leaves a too-small buffer untouched", "[codec][encode]",
                   LimitOrderMessage, MarketOrderMessage, CancelMessage,
                   AckMessage, RejectMessage, FillMessage, CancelledMessage) {
    TestType msg{};
    msg.type = static_cast<MessageType>(0xAB);  // any non-zero byte, so a partial write would show up
    std::array<std::uint8_t, sizeof(TestType) - 1> buffer{};

    (void)encode(msg, buffer); // (void) to tell the compiier we are aware of not assigning return value

    for (auto byte : buffer) {
        REQUIRE(byte == 0); // buffer should remain unwritten in
    }
}

// ===========================================================================
// decode
// ===========================================================================

namespace {

// Encodes a message into an exactly-sized array. Fails the test if encode
// doesn't write the whole message.
template <typename Message>
std::array<std::uint8_t, sizeof(Message)> to_bytes(const Message& msg) {
    std::array<std::uint8_t, sizeof(Message)> bytes{};
    REQUIRE(encode(msg, bytes) == sizeof(Message));
    return bytes;
}

LimitOrderMessage valid_limit() {
    return {MessageType::LimitOrder, Side::Buy, 7, 100, 10050};
}
MarketOrderMessage valid_market() {
    return {MessageType::MarketOrder, Side::Sell, 8, 250};
}
CancelMessage valid_cancel() {
    return {MessageType::Cancel, 9};
}

void require_incomplete(std::span<const std::uint8_t> bytes) {
    const auto result = decode(bytes);
    REQUIRE(result.status == DecodeResult::Status::Incomplete);
    REQUIRE(result.bytes_consumed == 0);
}

void require_unparseable(std::uint8_t type_byte) {
    std::array<std::uint8_t, 32> bytes{};  // plenty of bytes, so length can't be the reason
    bytes[0] = type_byte;
    const auto result = decode(bytes);
    REQUIRE(result.status == DecodeResult::Status::Unparseable);
    REQUIRE(result.bytes_consumed == 0);
}

// A valid message decodes to the same type, the same bytes, and consumes exactly
// its own length.
template <typename Message>
void require_round_trip(const Message& msg) {
    const auto bytes  = to_bytes(msg);
    const auto result = decode(bytes);

    REQUIRE(result.status == DecodeResult::Status::Ok);
    REQUIRE(result.bytes_consumed == sizeof(Message));
    REQUIRE(std::holds_alternative<Message>(result.message));
    REQUIRE(to_bytes(std::get<Message>(result.message)) == bytes);
}

// Everything the gateway relies on when it has to send a Reject.
template <typename Message>
void require_invalid(const Message& msg, RejectReason expected_reason) {
    const std::uint32_t sent_id = msg.client_order_id;  // copied out: see note on packed fields
    const auto bytes  = to_bytes(msg);
    const auto result = decode(bytes);

    REQUIRE(result.status == DecodeResult::Status::InvalidField);
    REQUIRE(result.reason == expected_reason);
    REQUIRE(result.client_order_id == sent_id);
    REQUIRE(result.bytes_consumed == sizeof(Message));  // must skip the whole message
    REQUIRE(std::holds_alternative<std::monostate>(result.message));
}

}  // namespace

// --- Incomplete ------------------------------------------------------------

TEST_CASE("decode waits for more bytes when a message is incomplete", "[codec][decode]") {
    SECTION("empty buffer") {
        require_incomplete({});
    }

    SECTION("only the type byte has arrived") {
        for (auto type : {MessageType::LimitOrder, MessageType::MarketOrder, MessageType::Cancel}) {
            const std::array<std::uint8_t, 1> one{static_cast<std::uint8_t>(type)};
            require_incomplete(one);
        }
    }

    SECTION("one byte short of a limit order") {
        const auto bytes = to_bytes(valid_limit());
        require_incomplete(std::span(bytes).first(bytes.size() - 1));
    }

    SECTION("one byte short of a market order") {
        const auto bytes = to_bytes(valid_market());
        require_incomplete(std::span(bytes).first(bytes.size() - 1));
    }

    SECTION("one byte short of a cancel") {
        const auto bytes = to_bytes(valid_cancel());
        require_incomplete(std::span(bytes).first(bytes.size() - 1));
    }
}

// --- Unparseable -----------------------------------------------------------

TEST_CASE("decode treats an unrecognised type byte as Unparseable", "[codec][decode]") {
    SECTION("bytes that are not a message type") {
        const std::array<std::uint8_t, 4> unknown{0x00, 0x04, 0x2A, 0xFF};
        for (auto type_byte : unknown) {
            require_unparseable(type_byte);
        }
    }

    SECTION("outbound message types are not accepted inbound") {
        for (auto type : {MessageType::Ack, MessageType::Reject,
                          MessageType::Fill, MessageType::Cancelled}) {
            require_unparseable(static_cast<std::uint8_t>(type));
        }
    }
}

// --- Ok --------------------------------------------------------------------

TEST_CASE("decode accepts each valid inbound message", "[codec][decode]") {
    SECTION("limit order")  { require_round_trip(valid_limit()); }
    SECTION("market order") { require_round_trip(valid_market()); }
    SECTION("cancel")       { require_round_trip(valid_cancel()); }
}

TEST_CASE("decode reads the worked example in PROTOCOL.md", "[codec][decode]") {
    // Typed in from the spec, not produced by encode.
    const std::array<std::uint8_t, 14> bytes{
        0x01, 0x01, 0x07, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x42, 0x27, 0x00, 0x00,
    };
    const auto result = decode(bytes);

    REQUIRE(result.status == DecodeResult::Status::Ok);
    REQUIRE(result.bytes_consumed == 14);
    REQUIRE(std::holds_alternative<LimitOrderMessage>(result.message));

    const auto& msg = std::get<LimitOrderMessage>(result.message);
    const Side          side  = msg.side;
    const std::uint32_t id    = msg.client_order_id;
    const std::uint32_t qty   = msg.quantity;
    const std::int32_t  price = msg.price;

    REQUIRE(side == Side::Buy);
    REQUIRE(id == 7);
    REQUIRE(qty == 100);
    REQUIRE(price == 10050);
}

TEST_CASE("decode accepts field values exactly at their limits", "[codec][decode]") {
    auto msg = valid_limit();

    SECTION("sell side")        { msg.side = Side::Sell;        require_round_trip(msg); }
    SECTION("minimum quantity") { msg.quantity = min_quantity;  require_round_trip(msg); }
    SECTION("maximum quantity") { msg.quantity = max_quantity;  require_round_trip(msg); }
    SECTION("minimum price")    { msg.price = min_price;        require_round_trip(msg); }
    SECTION("maximum price")    { msg.price = max_price;        require_round_trip(msg); }
}

TEST_CASE("decode takes only the first of two back-to-back messages", "[codec][decode]") {
    const auto limit  = to_bytes(valid_limit());
    const auto cancel = to_bytes(valid_cancel());

    std::array<std::uint8_t, sizeof(LimitOrderMessage) + sizeof(CancelMessage)> stream{};
    std::copy(limit.begin(), limit.end(), stream.begin());
    std::copy(cancel.begin(), cancel.end(), stream.begin() + limit.size());

    const auto first = decode(stream);
    REQUIRE(first.status == DecodeResult::Status::Ok);
    REQUIRE(first.bytes_consumed == sizeof(LimitOrderMessage));
    REQUIRE(std::holds_alternative<LimitOrderMessage>(first.message));

    // What the gateway will do: skip what was consumed, decode what's left.
    const auto second = decode(std::span(stream).subspan(first.bytes_consumed));
    REQUIRE(second.status == DecodeResult::Status::Ok);
    REQUIRE(second.bytes_consumed == sizeof(CancelMessage));
    REQUIRE(std::holds_alternative<CancelMessage>(second.message));
}

// --- InvalidField ----------------------------------------------------------

TEST_CASE("decode rejects a limit order with an out-of-range field", "[codec][decode]") {
    auto msg = valid_limit();

    SECTION("side zero") {
        msg.side = static_cast<Side>(0);
        require_invalid(msg, RejectReason::InvalidSide);
    }
    SECTION("side with no name") {
        msg.side = static_cast<Side>(3);
        require_invalid(msg, RejectReason::InvalidSide);
    }
    SECTION("client order id zero") {
        msg.client_order_id = 0;
        require_invalid(msg, RejectReason::UnknownClientID);
    }
    SECTION("quantity below minimum") {
        msg.quantity = min_quantity - 1;
        require_invalid(msg, RejectReason::QtyOutOfRange);
    }
    SECTION("quantity above maximum") {
        msg.quantity = max_quantity + 1;
        require_invalid(msg, RejectReason::QtyOutOfRange);
    }
    SECTION("price below minimum") {
        msg.price = min_price - 1;
        require_invalid(msg, RejectReason::PriceOutOfRange);
    }
    SECTION("negative price") {
        msg.price = -10050;
        require_invalid(msg, RejectReason::PriceOutOfRange);
    }
    SECTION("price above maximum") {
        msg.price = max_price + 1;
        require_invalid(msg, RejectReason::PriceOutOfRange);
    }
}

TEST_CASE("decode rejects a market order with an out-of-range field", "[codec][decode]") {
    auto msg = valid_market();

    SECTION("bad side") {
        msg.side = static_cast<Side>(3);
        require_invalid(msg, RejectReason::InvalidSide);
    }
    SECTION("client order id zero") {
        msg.client_order_id = 0;
        require_invalid(msg, RejectReason::UnknownClientID);
    }
    SECTION("quantity below minimum") {
        msg.quantity = min_quantity - 1;
        require_invalid(msg, RejectReason::QtyOutOfRange);
    }
    SECTION("quantity above maximum") {
        msg.quantity = max_quantity + 1;
        require_invalid(msg, RejectReason::QtyOutOfRange);
    }
}

TEST_CASE("decode rejects a cancel with client order id zero", "[codec][decode]") {
    auto msg = valid_cancel();
    msg.client_order_id = 0;
    require_invalid(msg, RejectReason::UnknownClientID);
}

TEST_CASE("decode reports the first invalid field in spec order", "[codec][decode]") {
    auto msg = valid_limit();

    SECTION("bad side and bad quantity: side wins") {
        msg.side     = static_cast<Side>(3);
        msg.quantity = 0;
        require_invalid(msg, RejectReason::InvalidSide);
    }
    SECTION("bad quantity and bad price: quantity wins") {
        msg.quantity = 0;
        msg.price    = 0;
        require_invalid(msg, RejectReason::QtyOutOfRange);
    }
}