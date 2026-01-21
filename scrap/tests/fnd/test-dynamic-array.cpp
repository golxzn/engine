#include <algorithm>
#include <array>

#include <catch2/catch_test_macros.hpp>
#include <gzn/fnd/containers/dynamic-array.hpp>

TEST_CASE("test: gzn::fnd::dynamic_array", "[fnd][dynamic-array]") {
  using namespace gzn;

  fnd::base_allocator alloc{};

  SECTION("constructors") {
    std::array constexpr init_list{ 1.0, 2.0, 3.0 };

    fnd::dynamic_array<double> const arr0{ alloc, 10 };
    REQUIRE(arr0.size() == 0);
    REQUIRE(arr0.capacity() == 10);


    fnd::dynamic_array<double> const arr1{ alloc, init_list };
    REQUIRE(arr1.size() == 3);
    REQUIRE(arr1.capacity() == 3);
    REQUIRE(
      std::equal(std::begin(arr1), std::end(arr1), std::begin(init_list))
    );


    fnd::dynamic_array<double> arr2{
      alloc, { 1.0, 2.0, 3.0 }
    };
    REQUIRE(arr2.size() == 3);
    REQUIRE(arr2.capacity() == 3);
    REQUIRE(std::equal(std::begin(arr2), std::end(arr2), std::begin(arr1)));


    fnd::dynamic_array<double> const arr3{ arr2 };
    REQUIRE(arr3.size() == 3);
    REQUIRE(arr3.capacity() == 3);
    REQUIRE(std::equal(std::begin(arr3), std::end(arr3), std::begin(arr2)));


    fnd::dynamic_array<double> const arr4{ std::move(arr2) };
    REQUIRE(arr4.size() == 3);
    REQUIRE(arr4.capacity() == 3);

    REQUIRE(arr2.data() == nullptr);
    REQUIRE(arr2.size() == 0);
    REQUIRE(arr2.capacity() == 0);

  } // SECTION("constructors")

  struct grow_info {
    size_t size{}, capacity{};
  };

  SECTION("growing") {
    fnd::dynamic_array<size_t> growing{ alloc };

    REQUIRE(growing.data() == nullptr);
    REQUIRE(growing.size() == 0);
    REQUIRE(growing.capacity() == 0);

    std::array constexpr expecting_results{
      grow_info{ 1,  1 },
      grow_info{ 2,  3 },
      grow_info{ 3,  3 },
      grow_info{ 4,  7 },
      grow_info{ 5,  7 },
      grow_info{ 6,  7 },
      grow_info{ 7,  7 },
      grow_info{ 8, 15 }
    };
    for (auto const expecting : expecting_results) {
      growing.push_back(expecting.size);
      REQUIRE(growing.size() == expecting.size);
      REQUIRE(growing.capacity() == expecting.capacity);
    }

  } // SECTION("growing")

  SECTION("reserve/reset/resize") {
    u32 constexpr new_size{ 4 };
    fnd::dynamic_array<grow_info> arr0{ alloc };

    arr0.reserve(new_size);
    REQUIRE(arr0.data() != nullptr);
    REQUIRE(arr0.size() == 0);
    REQUIRE(arr0.capacity() == new_size);

    // case 1, when m_size < count
    arr0.resize(new_size, grow_info{ .size = 1, .capacity = 1 });
    REQUIRE(arr0.data() != nullptr);
    REQUIRE(arr0.size() == new_size);
    REQUIRE(arr0.capacity() == new_size);

    // case 0, when m_size == count
    auto const old_data{ std::data(arr0) };
    arr0.resize(new_size);
    REQUIRE(arr0.data() == old_data);
    REQUIRE(arr0.size() == new_size);
    REQUIRE(arr0.capacity() == new_size);

    // case 3, when m_size > count
    arr0.resize(new_size / 2);
    REQUIRE(arr0.data() == old_data);
    REQUIRE(arr0.size() == new_size / 2);
    REQUIRE(arr0.capacity() == new_size);

    arr0.shrink_to_fit();
    REQUIRE(arr0.data() != old_data);
    REQUIRE(arr0.size() == arr0.capacity());

  } // SECTION("reserve/reset/resize")

  SECTION("data manipulation") {
    fnd::dynamic_array<grow_info> arr0{ alloc };

    REQUIRE(arr0.empty());

    arr0.emplace_back(100u, 100u);
    arr0.emplace_back(100u, 200u);
    REQUIRE(arr0.size() == 2);
    REQUIRE(arr0.front() != arr0.back());
    REQUIRE(arr0.front()->size == arr0.back()->size);
    REQUIRE(arr0.front()->capacity != arr0.back()->capacity);

    REQUIRE(arr0[0].size == arr0[1].size);
    REQUIRE(arr0[0].capacity != arr0[1].capacity);

    arr0.pop_back();
    REQUIRE(arr0.size() == 1);
    REQUIRE(arr0.size_in_bytes() == sizeof(grow_info));
    REQUIRE(arr0.capacity_in_bytes() == 3 * sizeof(grow_info));

    arr0.reserve(7);
    for (size_t i{}; i < 5; ++i) { arr0.emplace_back(i, i); }

    auto const expected{ arr0[4] };
    auto const next{ arr0.erase(arr0.at(3)) };
    REQUIRE(next->size == expected.size);
    REQUIRE(next->capacity == expected.capacity);

  } // SECTION("data manipulation (push/pop/at)")

} // TEST_CASE("common", "[raw-data]")
