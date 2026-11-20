#pragma once

#include <string>
#include <utility>

#include <tl/expected.hpp>

namespace nexum::core {

// Why an operation failed, in words a developer can act on.
struct Error {
    std::string message;
};

// The return type of every operation that can fail in an expected way (INV-14).
// tl::expected has the interface of std::expected; the switch waits for C++23 (docs/TECH_DEBT.md, TD-0004).
template <typename Value>
using Result = tl::expected<Value, Error>;

[[nodiscard]] inline tl::unexpected<Error> MakeError(std::string message) {
    return tl::unexpected<Error>(Error{std::move(message)});
}

} // namespace nexum::core
