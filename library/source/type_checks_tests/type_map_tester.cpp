#include <catch.hpp>
#include "darcel/syntax/syntax_builders.hpp"
#include "darcel/type_checks/type_map.hpp"

using namespace darcel;

TEST_CASE("test_variable_type_map", "[type_map]") {
  type_map m;
  auto v = std::make_shared<variable>(location::global(), "x");
  m.add(*v, std::make_shared<integer_data_type>());
  REQUIRE(*m.get_type(*v) == integer_data_type());
}
