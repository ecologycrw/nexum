// The ping of the client and its time (CA-08): the plain ping of a real session, with the counter 10 on the
// wire (finding 2026-11-21-capturas-reais-sessao-106-3.md, section 3), and the time in ms of the monotonic
// clock (docs/specs/protocolo/protocolo-106-3-m2.md, sections 6.6 and 8).
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Session/Ping.hpp"

namespace {

using mu::session::ClientMilliseconds;
using nexum::time::Seconds;

} // namespace

TEST_CASE("[0001-CA-08] the ping of a real session: the time little-endian and the speeds 29 and 29") {
    auto ping = mu::session::WritePing(0x08B7A69C, 29, 29);
    REQUIRE(ping.has_value());
    const std::vector<std::byte> expected{std::byte{0xC3}, std::byte{0x0C}, std::byte{0x0E}, std::byte{0x00},
                                          std::byte{0x9C}, std::byte{0xA6}, std::byte{0xB7}, std::byte{0x08},
                                          std::byte{0x1D}, std::byte{0x00}, std::byte{0x1D}, std::byte{0x00}};
    CHECK(std::vector<std::byte>(ping->begin(), ping->end()) == expected);
}

TEST_CASE("[0001-CA-08] the time of the ping is the monotonic time in ms, going round at 2^32") {
    CHECK(ClientMilliseconds(Seconds{0.0}) == 0);
    CHECK(ClientMilliseconds(Seconds{20.0}) == 20000);
    CHECK(ClientMilliseconds(Seconds{0.0015}) == 1);
    CHECK(ClientMilliseconds(Seconds{4294968.0}) == 704);
    CHECK(ClientMilliseconds(Seconds{-1.0}) == 0);
    CHECK(ClientMilliseconds(Seconds{std::numeric_limits<double>::quiet_NaN()}) == 0);
    CHECK(ClientMilliseconds(Seconds{std::numeric_limits<double>::infinity()}) == 0);
}

TEST_CASE("[0001-CA-08] the clock of the ping waits for its start, and stops") {
    mu::session::PingClock clock;
    CHECK_FALSE(clock.Due(Seconds{100.0}).has_value());
    clock.Start(Seconds{100.0});
    CHECK_FALSE(clock.Due(Seconds{119.9}).has_value());
    CHECK(clock.Due(Seconds{120.0}).has_value());
    clock.Stop();
    CHECK_FALSE(clock.Due(Seconds{140.0}).has_value());
}
