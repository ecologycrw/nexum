// The messages that the session holds until the tick takes them: in order, each with its epoch, and never
// dropped, even past the reserved room (IV.14.1 and IV.14.6 of the reference spec).
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Session/MessageQueue.hpp"

namespace {

using mu::session::MessageQueue;
using mu::session::SessionEpoch;
using Bytes = std::vector<std::byte>;

struct Taken {
    SessionEpoch epoch;
    Bytes message;
};

[[nodiscard]] std::vector<Taken> DrainAll(MessageQueue& queue) {
    std::vector<Taken> taken;
    queue.Drain([&](SessionEpoch epoch, std::span<const std::byte> message) {
        taken.push_back({epoch, Bytes(message.begin(), message.end())});
    });
    return taken;
}

} // namespace

TEST_CASE("[0001-CA-07] the queue gives the messages in the order they came, each with its epoch") {
    MessageQueue queue(64, 4);
    CHECK(queue.Push(SessionEpoch{1}, Bytes{std::byte{1}, std::byte{2}}));
    CHECK(queue.Push(SessionEpoch{2}, Bytes{std::byte{3}}));
    CHECK(queue.Count() == 2);

    const std::vector<Taken> taken = DrainAll(queue);
    REQUIRE(taken.size() == 2);
    CHECK(taken[0].epoch == SessionEpoch{1});
    CHECK(taken[0].message == Bytes{std::byte{1}, std::byte{2}});
    CHECK(taken[1].epoch == SessionEpoch{2});
    CHECK(taken[1].message == Bytes{std::byte{3}});
    CHECK(queue.Empty());
    CHECK(DrainAll(queue).empty());
}

TEST_CASE("[0001-CA-07] past the reserved room the queue grows and says so, without dropping a message") {
    MessageQueue queue(4, 2);
    CHECK(queue.Push(SessionEpoch{1}, Bytes(4, std::byte{7})));
    CHECK_FALSE(queue.Push(SessionEpoch{1}, Bytes(3, std::byte{8})));
    CHECK_FALSE(queue.Push(SessionEpoch{1}, Bytes(1, std::byte{9})));
    const std::vector<Taken> taken = DrainAll(queue);
    REQUIRE(taken.size() == 3);
    CHECK(taken[1].message == Bytes(3, std::byte{8}));
    CHECK(taken[2].message == Bytes(1, std::byte{9}));
}
