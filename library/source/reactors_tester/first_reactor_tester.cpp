#include <catch.hpp>
#include "darcel/reactors/first_reactor.hpp"
#include "darcel/reactors_tester/assertions.hpp"

using namespace darcel;
using namespace darcel::tests;

TEST_CASE("test_first_constant", "[first_reactor]") {
  auto reactor = first(123);
  assert_value(*reactor, 0, base_reactor::update::COMPLETE_EVAL, 123);
}