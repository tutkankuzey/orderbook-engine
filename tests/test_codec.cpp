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