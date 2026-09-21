#include "orderbook/protocol/messages.h"
#include "orderbook/protocol/codec.h"
#include <cstring>
#include <sys/types.h>

namespace orderbook::protocol{


    DecodeResult decode(std::span<const std::uint8_t> buffer){

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