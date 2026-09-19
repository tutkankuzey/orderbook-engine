# Exchange Wire Protocol v1

## Overview

This document specifies the binary wire protocol for sending orders to the
exchange and receiving responses. It is intended for anyone implementing a
client.

The protocol is binary, with a fixed layout per message type. It runs over TCP,
one connection per client. Each connection carries only that client's traffic.

## Conventions

**Byte order.** Little-endian: the least significant byte of a multi-byte
integer is at the lowest offset. This differs from the big-endian convention of
older network protocols; it is chosen because both the exchange and every
expected client run on little-endian hardware, so no conversion is needed on
either side.

**Offsets** are counted in bytes from the start of the message, beginning at 0.

**Integer types** are written as `uint8`, `uint32`, `int32`, `uint64`. Signed
values are two's complement.

**Prices** are integer ticks with a multiplier of 100, so one tick is one cent
and `$100.50` is transmitted as `10050`. The multiplier is a property of this
protocol version and is not configurable.

**Reserved values.** The value `0` is not a legal message type, side, client
order ID, quantity or price. A zero where a value is required is always a
malformed message. This makes an all-zero buffer — the most common result of a
client bug — detectable rather than silently valid.

## Framing

The first byte of every message is its type. The type determines the total
length of the message, so a reader takes one byte, looks up the length, and
reads that many further bytes.

### Inbound message types (client to exchange)

| Type byte | Message | Total length |
|---|---|---|
| `1` | Limit order | 14 |
| `2` | Market order | 10 |
| `3` | Cancel | 5 |

### Outbound message types (exchange to client)

| Type byte | Message | Total length |
|---|---|---|
| `101` | Ack | 13 |
| `102` | Reject | 6 |
| `103` | Fill | 22 |

Inbound and outbound types occupy separate number ranges so that a message seen
in the wrong direction is immediately detectable.

## Shared field meanings

**Client Order ID.** A `uint32` chosen by the client to identify one order.

- Must be unique among all orders the client has sent on the current
  connection, whether or not those orders are still live.
- Must not be `0`.
- Scope is the connection: two different clients may both use `7`, and the same
  client may reuse `7` after reconnecting.
- Recommended implementation: a counter starting at `1`, incremented for each
  order sent. Do not reuse an ID after an order is filled or cancelled — the
  exchange may still send messages referring to it.
- Its purpose is to let the client refer to an order in the interval between
  sending it and receiving the Ack that assigns an exchange ID. A cancel sent
  during that interval can only identify the order by its client ID.

**Exchange Order ID.** A `uint64` assigned by the exchange, unique across all
orders from all clients, never reused. Delivered in the Ack. Client IDs cannot
serve this purpose because they collide between clients.

**Side.** `1` = Buy, `2` = Sell. Present only on Limit and Market orders; a
Cancel does not carry a side, because the order being cancelled already has one.

## Inbound messages

### Limit order (type 1, 14 bytes)

| Offset | Size | Type | Name | Legal values |
|---|---|---|---|---|
| 0 | 1 | `uint8` | Message Type | `1` |
| 1 | 1 | `uint8` | Side | `1` or `2` |
| 2 | 4 | `uint32` | Client Order ID | `1` to `4294967295` |
| 6 | 4 | `uint32` | Quantity | `1` to `100000` |
| 10 | 4 | `int32` | Price | `1` to `100000000` |

Price is declared signed to match the engine's internal representation, so that
a client bug sending a negative value produces a recognisably negative number
rather than a value near four billion. Negative and zero prices are rejected.

### Market order (type 2, 10 bytes)

| Offset | Size | Type | Name | Legal values |
|---|---|---|---|---|
| 0 | 1 | `uint8` | Message Type | `2` |
| 1 | 1 | `uint8` | Side | `1` or `2` |
| 2 | 4 | `uint32` | Client Order ID | `1` to `4294967295` |
| 6 | 4 | `uint32` | Quantity | `1` to `100000` |

A market order carries no price. Rather than padding the message with an unused
price field, the message is simply shorter; the type byte already tells the
reader how many bytes to expect.

### Cancel (type 3, 5 bytes)

| Offset | Size | Type | Name | Legal values |
|---|---|---|---|---|
| 0 | 1 | `uint8` | Message Type | `3` |
| 1 | 4 | `uint32` | Client Order ID | `1` to `4294967295` |

The Client Order ID identifies the order to cancel. A cancel may be sent before
the Ack for that order has been received.

## Outbound messages

### Ack (type 101, 13 bytes)

Sent when an order is accepted into the book. It is the only message that
carries both identifiers, and is therefore how a client learns the mapping
between its own ID and the exchange's.

| Offset | Size | Type | Name | Notes |
|---|---|---|---|---|
| 0 | 1 | `uint8` | Message Type | `101` |
| 1 | 4 | `uint32` | Client Order ID | Echoed from the order |
| 5 | 8 | `uint64` | Exchange Order ID | Assigned by the exchange |

An Ack means accepted, not executed. An order may be acked and then filled, or
acked and rest indefinitely. A market order that executes immediately is still
acked first.

### Reject (type 102, 6 bytes)

| Offset | Size | Type | Name | Notes |
|---|---|---|---|---|
| 0 | 1 | `uint8` | Message Type | `102` |
| 1 | 4 | `uint32` | Client Order ID | Echoed from the order |
| 5 | 1 | `uint8` | Reason Code | See below |

| Reason code | Meaning |
|---|---|
| `1` | Invalid side |
| `2` | Quantity out of range |
| `3` | Price out of range |
| `4` | Duplicate Client Order ID |
| `5` | Unknown Client Order ID (cancel for an order that is not live) |
| `6` | No liquidity (market order with an empty opposing book) |

A numeric code rather than free text keeps the message fixed-size and avoids
clients parsing prose. New codes may be added in later versions; a client that
receives an unknown code should treat the order as rejected.

### Fill (type 103, 22 bytes)

| Offset | Size | Type | Name | Notes |
|---|---|---|---|---|
| 0 | 1 | `uint8` | Message Type | `103` |
| 1 | 4 | `uint32` | Client Order ID | Echoed from the order |
| 5 | 8 | `uint64` | Exchange Order ID | The filled order |
| 13 | 4 | `int32` | Fill Price | Ticks; the resting order's price |
| 17 | 4 | `uint32` | Fill Quantity | This execution only, not cumulative |
| 21 | 1 | `uint8` | Remaining Flag | `0` = order fully filled, `1` = partially filled, remainder still live |

Both identifiers are carried: the client ID so the client can match the fill
without keeping a mapping table, and the exchange ID so it is unambiguous if a
client has reused an ID incorrectly. One order may produce several Fill
messages, each for one execution, since an aggressive order can match several
resting orders at different prices.

The remaining flag exists so a client can tell a completed order from a partial
one without accumulating quantities itself.

## Error handling

Three tiers, distinguished by whether the exchange still knows where it is in
the byte stream.

**Tier 1 — valid message, rejected by the engine.** The message parses and its
fields are in range, but the engine declines it: a cancel for an unknown order,
a market order with no opposing liquidity, a duplicate client ID. Response: a
Reject with the appropriate code. The connection continues.

**Tier 2 — well-formed structure, invalid field values.** The type byte is
known, so the message length is known and it was read in full, but a field is
out of range: quantity `0`, side `7`, negative price. Response: a Reject with
the appropriate code. The connection continues, because the message boundary is
known and the reader is still synchronised.

**Tier 3 — unparseable.** The type byte is not a known inbound type. The
message length is therefore unknown, so the reader cannot find where the next
message begins, and no subsequent byte can be trusted. There is no recovery
mechanism. Response: close the connection immediately without a Reject.

Since TCP delivers bytes in order and uncorrupted, a tier-3 error means the
client's encoder is broken or the client is hostile. Disconnection is also the
resynchronisation mechanism: a reconnecting client starts a fresh stream whose
first byte is, by definition, the start of a message.

A client may also stop sending mid-message. The exchange does not treat a
partial message as an error while it waits, but a connection with an incomplete
message and no further data may be closed after a timeout. Timeout handling is
not specified in this version.

## Worked example

A limit order to buy 100 shares at `$100.50`, with client order ID 7.

| Field | Value | Bytes (hex, in order) |
|---|---|---|
| Message Type | 1 | `01` |
| Side | Buy | `01` |
| Client Order ID | 7 | `07 00 00 00` |
| Quantity | 100 | `64 00 00 00` |
| Price | 10050 | `42 27 00 00` |

Full message: `01 01 07 00 00 00 64 00 00 00 42 27 00 00` — 14 bytes.

Note the price: 10050 is `0x2742`, transmitted least significant byte first as
`42 27 00 00`.

## Design notes

**Binary rather than text.** The exchange already has a CSV format for replay,
which is a text protocol. Binary is used on the wire because fields sit at fixed
offsets, so reading a price is a copy rather than a scan for the fourth
delimiter; because sizes are constant regardless of value; and because text
admits ambiguities (leading zeros, whitespace, decimal points) that a binary
format cannot express. Text protocols remain common for order entry at lower
message rates — FIX is the industry example — where readability matters more
than nanoseconds.

**Fixed size per type rather than a length prefix.** The type byte implies the
length, which removes a field and one class of validation. A length prefix would
allow variable-length messages and let a reader skip message types it does not
understand; neither is needed here. It would also allow rejecting an implausible
length early, which is a genuine if minor advantage.

**Fixed-size fields rather than variable-length encoding.** FAST encodes integers 
in as few bytes as possible using a stop bit, and transmits only fields that changed. 
That design trades CPU work for bandwidth, which was the binding constraint for market 
data feeds in 2006. This exchange is optimising for latency, and variable-length fields 
would destroy the fixed-offset property that makes decoding a copy rather than a scan.

**Shorter messages rather than padding.** A cancel is 5 bytes rather than 14
padded with zeros. Since the type byte already determines the length, there
is no padding taking place.

**No bit packing.** Side could occupy two bits of the type byte. It gets its own
byte instead: at these message sizes the saving is zero once alignment is
considered, and every read would need a mask and a shift.

**Separate type ranges for the two directions.** Inbound types are `1`–`3` and
outbound types start at `101`, so a message travelling the wrong way is easily
caught.

## Out of scope for v1

- **Single instrument.** No symbol or instrument field; the exchange trades one
  instrument and both sides know which.
- **No authentication.** Any process that can open the socket can trade.
- **No sequence numbers on order entry.** TCP guarantees ordering within a
  connection, and there is no retransmission mechanism to make sequence numbers
  useful. The market data feed, which is UDP multicast, will require them.
- **No heartbeats or timeouts.** A silent connection is not distinguished from
  a dead one.
- **No session resumption.** A reconnecting client starts fresh; the exchange
  does not replay missed messages.
- **No order modification.** Cancel and resend.
- **No protocol version negotiation.** Both sides are assumed to implement v1.
