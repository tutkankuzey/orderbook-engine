#include "orderbook/protocol/codec.h"
#include "orderbook/protocol/messages.h"
#include <cstdint>
#include <cstring>
#include <variant>

namespace orderbook::protocol{
    namespace {

    template <typename Message>
    DecodeResult invalid_field(const Message& msg, RejectReason reason) {
        return DecodeResult{
            .status          = DecodeResult::Status::InvalidField,
            .bytes_consumed  = sizeof(Message),
            .client_order_id = msg.client_order_id,
            .reason          = reason,
        };
    }

    } // anonymous namespace

    DecodeResult decode(std::span<const std::uint8_t> buffer){
        if (buffer.empty()) return DecodeResult{}; // Defaults to Incomplete status and monostate for std::variant

        const auto type = static_cast<MessageType>(buffer[0]);
        switch (type) {
            case MessageType::LimitOrder:{
                if (buffer.size() < sizeof(LimitOrderMessage)) return DecodeResult{};
                
                LimitOrderMessage msg;
                std::memcpy(&msg, buffer.data(), sizeof(LimitOrderMessage));

                if (msg.side != Side::Buy && msg.side != Side::Sell){
                   return invalid_field(msg, RejectReason::InvalidSide);
                }

                if (msg.client_order_id == 0){
                    return invalid_field(msg, RejectReason::UnknownClientID);
                }
                
                if (msg.quantity < min_quantity || msg.quantity > max_quantity){
                    return invalid_field(msg, RejectReason::QtyOutOfRange);
                }

                if (msg.price < min_price || msg.price > max_price){
                    return invalid_field(msg, RejectReason::PriceOutOfRange);
                }

                // Passed all checks
                return DecodeResult{
                    .status = DecodeResult::Status::Ok,
                    .bytes_consumed = sizeof(LimitOrderMessage),
                    .client_order_id = msg.client_order_id,
                    .message = msg};
            }


            case MessageType::MarketOrder:{
                if (buffer.size() < sizeof(MarketOrderMessage)) return DecodeResult{};

                MarketOrderMessage msg;
                std::memcpy(&msg, buffer.data(), sizeof(MarketOrderMessage));

                if (msg.side != Side::Buy && msg.side != Side::Sell){
                   return invalid_field(msg, RejectReason::InvalidSide);
                }

                if (msg.client_order_id == 0){
                    return invalid_field(msg, RejectReason::UnknownClientID);
                }
                
                if (msg.quantity < min_quantity || msg.quantity > max_quantity){
                    return invalid_field(msg, RejectReason::QtyOutOfRange);
                }

                return DecodeResult{
                    .status = DecodeResult::Status::Ok,
                    .bytes_consumed = sizeof(MarketOrderMessage),
                    .client_order_id = msg.client_order_id,
                    .message = msg};
            }

            case MessageType::Cancel:{
                if (buffer.size() < sizeof(CancelMessage)) return DecodeResult{};

                CancelMessage msg;
                std::memcpy(&msg, buffer.data(), sizeof(CancelMessage));
                
                if (msg.client_order_id == 0){
                    return invalid_field(msg, RejectReason::UnknownClientID);
                }

                return DecodeResult{
                    .status = DecodeResult::Status::Ok,
                    .bytes_consumed = sizeof(CancelMessage),
                    .client_order_id = msg.client_order_id,
                    .message = msg};
            }

            default: return DecodeResult{.status = DecodeResult::Status::Unparseable};
        }
    }

    std::size_t encode(const LimitOrderMessage& msg, std::span<std::uint8_t> out){
        constexpr std::size_t message_size = sizeof(LimitOrderMessage);

        if (out.size() < message_size) {
            return 0;
        }
        std::memcpy(out.data(), &msg, message_size);
        return message_size;
    }

    std::size_t encode(const MarketOrderMessage& msg, std::span<std::uint8_t> out){
        constexpr std::size_t message_size = sizeof(MarketOrderMessage);

        if (out.size() < message_size) {
            return 0;
        }
        std::memcpy(out.data(), &msg, message_size);
        return message_size;
    }

    std::size_t encode(const CancelMessage& msg, std::span<std::uint8_t> out){
        constexpr std::size_t message_size = sizeof(CancelMessage);

        if (out.size() < message_size) {
            return 0;
        }
        std::memcpy(out.data(), &msg, message_size);
        return message_size;
    }

    std::size_t encode(const AckMessage& msg, std::span<std::uint8_t> out){
        constexpr std::size_t message_size = sizeof(AckMessage);

        if (out.size() < message_size) {
            return 0;
        }
        std::memcpy(out.data(), &msg, message_size);
        return message_size;
    }

    std::size_t encode(const CancelledMessage& msg, std::span<std::uint8_t> out){
        constexpr std::size_t message_size = sizeof(CancelledMessage);

        if (out.size() < message_size) {
            return 0;
        }

        std::memcpy(out.data(), &msg, message_size);
        return message_size;

    }

    std::size_t encode(const RejectMessage& msg, std::span<std::uint8_t> out){
        constexpr std::size_t message_size = sizeof(RejectMessage);

        if (out.size() < message_size) {
            return 0;
        }
        std::memcpy(out.data(), &msg, message_size);
        return message_size;
    }

    std::size_t encode(const FillMessage& msg, std::span<std::uint8_t> out){
        constexpr std::size_t message_size = sizeof(FillMessage);

        if (out.size() < message_size) {
            return 0;
        }
        std::memcpy(out.data(), &msg, message_size);
        return message_size;
    }
} // namespace orderbook::protocol