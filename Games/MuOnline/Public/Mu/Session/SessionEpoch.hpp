#pragma once

// The epoch of the session, a value on its own, so that the queue of the events of the world (Mu/World) can
// keep it without knowing the session.

#include <compare>
#include <cstdint>

namespace mu::session {

// Counts the connections of the session: each new connection gets the next number, so an event of a
// connection that is already over is told apart and dropped (D-10 of the reference spec, item 5). From M4 on,
// each change of map also moves it. A smaller number is an older epoch.
struct SessionEpoch {
    std::uint32_t value = 0;

    friend constexpr bool operator==(SessionEpoch, SessionEpoch) noexcept = default;
    friend constexpr auto operator<=>(SessionEpoch, SessionEpoch) noexcept = default;
};

} // namespace mu::session
