#pragma once

#include <string>

namespace other_game {

// A game written by another developer: it only knows the public interfaces of the engine and of the kit.
[[nodiscard]] std::string DescribeFoundation();

} // namespace other_game
