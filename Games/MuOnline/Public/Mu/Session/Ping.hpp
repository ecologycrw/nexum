#pragma once

// The ping of the client (C3 0E 00, 12 plain bytes): every 20 seconds of the wall clock, from the entry in
// the Game Server, with the time in ms and the attack and magic speeds last known. The server answers
// nothing, and it closes a connection that sends no ping for 2 minutes after the first. Source: the protocol
// spec of the knowledge center (docs/specs/protocolo/protocolo-106-3-m2.md, sections 6.6 and 8) and the
// finding 2026-11-21-capturas-reais-sessao-106-3.md (section 4: the real client pings from the entry in the
// Game Server, still on the login screen, with the speeds 0 and 0 until the character enters).

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "Mu/Protocol/Dialect.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Time/Seconds.hpp"
#include "Nexum/Time/WallTimer.hpp"

namespace mu::session {

inline constexpr nexum::time::Seconds kPingPeriod{20.0};

using PingMessage = std::array<std::byte, protocol::dialect::Ping::kDeclaredLength>;

// The time that the client puts in the login and in the ping, in ms. The server only writes it to its log, so
// it is the monotonic time of the session, going round at 2^32; the login carries it big-endian and the ping
// little-endian (sections 6.6 and 8).
[[nodiscard]] std::uint32_t ClientMilliseconds(nexum::time::Seconds now) noexcept;

// The plain ping, before Xor32 and SimpleModulus.
[[nodiscard]] nexum::core::Result<PingMessage> WritePing(std::uint32_t milliseconds,
                                                         std::uint16_t attackSpeed, std::uint16_t magicSpeed);

// When the ping is due, and the speeds it carries. It runs on the monotonic time of each frame, not on frames
// or ticks, so it goes on while the map loads (D-05 and IV.16 of the reference spec).
class PingClock {
public:
    // The entry in the Game Server: the first ping is due one period later.
    void Start(nexum::time::Seconds now) noexcept;
    void Stop() noexcept;

    // The speeds of the last state known: the data of the character, then each update of its stats.
    void SetSpeeds(std::uint16_t attackSpeed, std::uint16_t magicSpeed) noexcept;

    // The ping to send at `now`, if one is due. At most one per call: the periods missed in a long stall
    // become a single ping, and the next stays on the same rhythm.
    [[nodiscard]] std::optional<nexum::core::Result<PingMessage>> Due(nexum::time::Seconds now);

private:
    std::optional<nexum::time::WallTimer> m_timer;
    std::uint16_t m_attackSpeed = 0;
    std::uint16_t m_magicSpeed = 0;
};

} // namespace mu::session
