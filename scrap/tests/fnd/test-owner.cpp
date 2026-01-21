#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <gzn/fnd/owner.hpp>

namespace game {

enum class debuff { self_flagellation, sorrowing, overthinking };

struct player {
  std::string         name;
  int                 hp{};
  int                 lvl{};
  std::vector<debuff> debuffs;
};

} // namespace game

template<class owner_type>
void testing(
  gzn::fnd::util::allocator_type auto &alloc,
  std::string_view const               title
) {
  using namespace gzn;

  std::initializer_list static constexpr mcli{
    game::debuff::self_flagellation, //
    game::debuff::sorrowing,
    game::debuff::overthinking
  };

  SECTION("construction") {
    REQUIRE_NOTHROW(owner_type{ alloc, "golxzn", 1, -1, mcli });
  } // SECTION("construction")

  SECTION("references") {
    owner_type owner{ alloc, "golxzn", 1, -1, mcli };

    REQUIRE(owner.is_alive());
    REQUIRE(owner.reference_count() == 1);

    REQUIRE_FALSE(std::empty(owner->name));
    REQUIRE(owner->hp == 1);
    REQUIRE(owner->lvl == -1);
    REQUIRE_FALSE(std::empty(owner->debuffs));

    fnd::ref r1{ owner };
    fnd::ref r2{ owner };
    fnd::ref r3{ owner };

    REQUIRE(r1.is_alive());
    REQUIRE(r2.is_alive());
    REQUIRE(r3.is_alive());
    REQUIRE(owner.reference_count() == 4);

    r1.unlink();
    REQUIRE_FALSE(r1.is_alive());
    REQUIRE(r1.reference_count() == 0);
    REQUIRE(owner.reference_count() == 3);
    REQUIRE(owner.reference_count() == r2.reference_count());
  } // SECTION("stack_owner references")

  SECTION("move") {
    owner_type owner{ alloc, "golxzn", 1, -1, mcli };
    fnd::ref   r1{ owner };

    owner_type other{ std::move(owner) };
    REQUIRE_FALSE(owner.is_alive());
    REQUIRE(owner.raw() == nullptr);
    REQUIRE(other.is_alive());
    REQUIRE(r1.is_alive());
    REQUIRE(r1->name == other->name);
  } // SECTION("move")

  SECTION("ref outlives owner") {
    fnd::ref<game::player> r1{};
    REQUIRE_FALSE(r1.is_alive());

    {
      owner_type owner{ alloc, "golxzn", 1, -1, mcli };
      r1 = owner;
      REQUIRE(r1.is_alive());
      REQUIRE(r1->name == owner->name);
    }

    REQUIRE_FALSE(r1.is_alive());
  } // SECTION("ref outlives owner")
}

TEST_CASE("test: gzn::stack_owner", "[fnd][owner]") {
  gzn::fnd::stack_arena_allocator<1024> alloc{};
  testing<gzn::fnd::stack_owner<game::player>>(alloc, "stack_owner");
} // TEST_CASE("common", "[raw-data]")

TEST_CASE("test: gzn::heap_owner", "[fnd][owner]") {
  gzn::fnd::base_allocator alloc{};
  testing<gzn::fnd::heap_owner<game::player>>(alloc, "stack_owner");
} // TEST_CASE("common", "[raw-data]")
