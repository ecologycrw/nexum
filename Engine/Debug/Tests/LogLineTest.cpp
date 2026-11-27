#include <cstdint>
#include <limits>
#include <string>

#include <doctest/doctest.h>

#include "Nexum/Debug/LogLine.hpp"

using nexum::debug::LogLine;

TEST_CASE("a line of the log joins texts and numbers") {
    LogLine line;
    line.Append("the object 0x").AppendHex(0x200, 4).Append(" walks to (").AppendNumber(144).Append(", ");
    line.AppendNumber(126).Append(")");
    CHECK(line.View() == "the object 0x0200 walks to (144, 126)");
    CHECK_FALSE(line.Cut());
}

TEST_CASE("the numbers of a line of the log cover the whole range") {
    LogLine line;
    line.AppendNumber(0).Append(" ").AppendNumber(std::numeric_limits<std::uint64_t>::max());
    line.Append(" ").AppendHex(0, 2).Append(" ").AppendHex(0x7E, 1).Append(" ").AppendHex(0xABCDEF, 2);
    line.Append(" ").AppendHex(std::numeric_limits<std::uint64_t>::max(), 40);
    CHECK(line.View() == "0 18446744073709551615 00 7E ABCDEF FFFFFFFFFFFFFFFF");
}

TEST_CASE("a line of the log longer than its room is cut, and cleared it starts again") {
    LogLine line;
    const std::string longText(LogLine::kRoom + 10, 'a');
    line.Append(longText);
    CHECK(line.View().size() == LogLine::kRoom);
    CHECK(line.Cut());
    line.AppendNumber(7);
    CHECK(line.View().size() == LogLine::kRoom);

    line.Clear();
    CHECK(line.View().empty());
    CHECK_FALSE(line.Cut());
    line.Append("again");
    CHECK(line.View() == "again");
}
