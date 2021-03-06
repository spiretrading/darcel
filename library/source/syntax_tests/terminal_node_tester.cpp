#include <catch.hpp>
#include "darcel/syntax/terminal_node.hpp"

using namespace darcel;

TEST_CASE("test_terminal_node", "[test_terminal_node]") {
  TerminalNode n(Location::global());
  REQUIRE(n.get_location() == Location::global());
}
