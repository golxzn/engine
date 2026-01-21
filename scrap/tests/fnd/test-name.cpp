#include <catch2/catch_test_macros.hpp>
#include <gzn/fnd/name.hpp>

TEST_CASE("test: gzn::fnd::name", "[fnd][owner]") {
  using namespace gzn;

  fnd::u8name_view constexpr comptime_name{ u8"golxzn" };
  REQUIRE(std::size(comptime_name) == 6);

  std::u8string    some_name{ u8"golxzn" };
  fnd::u8name_view runtime_name{ some_name };
  REQUIRE(runtime_name == comptime_name);

  fnd::u8name stack_name{ u8"golxzn" };
  REQUIRE(runtime_name == stack_name);

  fnd::u8name copied_name{ runtime_name };
  REQUIRE(runtime_name == copied_name);

  switch (stack_name.hash()) {
    using namespace fnd::name_literals;
    case u8"golxzn"_name_hash: REQUIRE(!std::empty(stack_name)); break;
  }

} // TEST_CASE("common", "[raw-data]")
