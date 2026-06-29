#include <algorithm>
#include <array>
#include <numeric>

#include <catch2/catch_test_macros.hpp>
#include <gzn/fnd/allocators/in-stack-allocator.hpp>
#include <gzn/fnd/containers/index-array.hpp>

TEST_CASE("test: gzn::fnd::index_array", "[fnd][index-array]") {
  using namespace gzn;

  fnd::heap_allocator alloc{};

  SECTION("default constructor") {
    fnd::index_array<int> const arr{ &alloc };

    REQUIRE(arr.data() == nullptr);
    REQUIRE(arr.size() == 0);
    REQUIRE(arr.capacity() == 0);
    REQUIRE(arr.empty());
    REQUIRE(arr.bytes_count() == 0);
  }

  SECTION("reserve constructor") {
    constexpr size_t reserve_count{ 16 };

    fnd::index_array<int> const arr{ &alloc, reserve_count };

    REQUIRE(arr.size() == 0);
    REQUIRE(arr.data() != nullptr);
    REQUIRE(arr.capacity() == reserve_count);
    REQUIRE(arr.empty());
  }

  SECTION("range constructor") {
    std::array constexpr values{ 10, 20, 30, 40 };

    fnd::index_array<int> const arr{ &alloc, values };

    REQUIRE(arr.size() == values.size());
    REQUIRE(arr.capacity() == values.size());
    REQUIRE(std::equal(arr.begin(), arr.end(), values.begin()));
  }

  SECTION("c-style array constructure") {
    fnd::index_array<int> const arr{
      &alloc, { 10, 20, 30, 40 }
    };

    REQUIRE(arr.size() == 4);
    REQUIRE(arr[0] == 10);
    REQUIRE(arr[1] == 20);
    REQUIRE(arr[2] == 30);
    REQUIRE(arr[3] == 40);
  }


  SECTION("copy constructor") {
    fnd::index_array<int> const src{
      &alloc, { 1, 2, 3, 4 }
    };
    fnd::index_array<int> const dst{ src };

    REQUIRE(dst.size() == src.size());
    REQUIRE(dst.capacity() == src.capacity());
    REQUIRE(std::equal(dst.begin(), dst.end(), src.begin()));
  }

  SECTION("move constructor") {
    std::array constexpr values{ 5, 6, 7 };

    fnd::index_array<int> src{ &alloc, values };

    auto const old_ptr{ src.data() };

    fnd::index_array<int> const moved{ std::move(src) };

    REQUIRE(moved.data() == old_ptr);
    REQUIRE(moved.size() == values.size());

    REQUIRE(src.size() == 0);
    REQUIRE(src.capacity() == 0);
    REQUIRE(src.data() != old_ptr);
  }

  SECTION("copy assignment") {
    std::array constexpr values{ 3, 2, 1 };

    fnd::index_array<int> src{ &alloc, values };
    fnd::index_array<int> dst{ &alloc };

    dst = src;

    REQUIRE(dst.size() == src.size());
    REQUIRE(dst.capacity() == src.capacity());
    REQUIRE(std::equal(dst.begin(), dst.end(), src.begin()));
  }

  SECTION("move assignment") {
    std::array constexpr values{ 9, 8, 7 };

    fnd::index_array<int> src{ &alloc, values };
    fnd::index_array<int> dst{ &alloc };

    auto const ptr{ src.data() };

    dst = std::move(src);

    REQUIRE(dst.data() == ptr);
    REQUIRE(dst.size() == values.size());

    REQUIRE(src.size() == 0);
    REQUIRE(src.capacity() == 0);
    REQUIRE(src.data() == nullptr);
  }

  SECTION("push_back growth") {
    fnd::index_array<size_t> arr{ &alloc };

    REQUIRE(arr.empty());

    std::array constexpr expected_capacity{
      size_t{ 1 },  size_t{ 4 },  size_t{ 4 },  size_t{ 4 },
      size_t{ 13 }, size_t{ 13 }, size_t{ 13 }, size_t{ 13 },
    };

    for (size_t i{}; i < expected_capacity.size(); ++i) {
      auto const idx{ arr.push_back(i) };

      REQUIRE(idx == i);
      REQUIRE(arr.size() == i + 1);
      REQUIRE(arr.capacity() == expected_capacity[i]);
      REQUIRE(arr[idx] == i);
    }
  }

  SECTION("emplace_back") {
    struct value {
      int a;
      int b;
    };

    fnd::index_array<value> arr{ &alloc };

    auto const idx0{ arr.emplace_back(1, 2) };
    auto const idx1{ arr.emplace_back(3, 4) };

    REQUIRE(idx0 == 0);
    REQUIRE(idx1 == 1);

    REQUIRE(arr.size() == 2);

    REQUIRE(arr[idx0].a == 1);
    REQUIRE(arr[idx0].b == 2);

    REQUIRE(arr[idx1].a == 3);
    REQUIRE(arr[idx1].b == 4);
  }

  SECTION("reserve") {
    fnd::index_array<int> arr{ &alloc };

    arr.reserve(32);

    REQUIRE(arr.capacity() == 32);
    REQUIRE(arr.size() == 0);

    auto const ptr{ arr.data() };

    arr.reserve(8);

    REQUIRE(arr.capacity() == 32);
    REQUIRE(arr.data() == ptr);
  }

  SECTION("shrink_to_fit") {
    fnd::index_array<int> arr{ &alloc };

    arr.reserve(32);

    for (int i{}; i < 10; ++i) { std::ignore = arr.push_back(i); }

    REQUIRE(arr.capacity() == 32);

    arr.shrink_to_fit();

    REQUIRE(arr.size() == 10);
    REQUIRE(arr.capacity() == 10);

    for (int i{}; i < 10; ++i) { REQUIRE(arr[i] == i); }
  }

  SECTION("reset") {
    fnd::index_array<int> arr{ &alloc };

    for (int i{}; i < 8; ++i) { std::ignore = arr.push_back(i); }

    auto const cap{ arr.capacity() };

    arr.reset();

    REQUIRE(arr.empty());
    REQUIRE(arr.capacity() == cap);
  }

  SECTION("pop_back") {
    fnd::index_array<int> arr{ &alloc };

    std::ignore = arr.push_back(10);
    std::ignore = arr.push_back(20);
    std::ignore = arr.push_back(30);

    REQUIRE(arr.size() == 3);

    arr.pop_back();

    REQUIRE(arr.size() == 2);
    REQUIRE(arr[0] == 10);
    REQUIRE(arr[1] == 20);
  }

  SECTION("erase last element") {
    fnd::index_array<int> arr{ &alloc };

    auto const idx0{ arr.push_back(10) };
    auto const idx1{ arr.push_back(20) };
    auto const idx2{ arr.push_back(30) };
    auto const idx3{ arr.push_back(40) };

    auto const moved{ arr.erase(idx1) };

    REQUIRE(arr.size() == 3);

    REQUIRE(arr[idx0] == 10);
    REQUIRE(arr[idx2] == 30);
    REQUIRE(arr[idx3] == 40);

    REQUIRE(moved == 3);
  }

  SECTION("handles") {
    fnd::index_array<int> arr{ &alloc };

    auto const idx0{ arr.push_back(100) };
    auto const idx1{ arr.push_back(200) };

    auto h0{ arr.make_handle(idx0) };
    auto h1{ arr.make_handle(idx1) };

    REQUIRE(h0.is_valid());
    REQUIRE(h1.is_valid());

    REQUIRE(h0.value() == 100);
    REQUIRE(h1.value() == 200);

    h0.value() = 123;

    REQUIRE(arr[0] == 123);

    arr.erase(idx0);

    REQUIRE_FALSE(h0.is_valid());
    REQUIRE(h1.is_valid());
  }

  SECTION("generation changes") {
    fnd::index_array<int> arr{ &alloc };

    auto const idx{ arr.push_back(1) };
    auto const gen0{ arr.generation(idx) };

    arr.erase(idx);

    auto const gen1{ arr.generation(idx) };

    REQUIRE(gen1 != gen0);
  }

  SECTION("begin/end") {
    fnd::index_array<int> arr{ &alloc };

    for (int i{}; i < 16; ++i) { std::ignore = arr.push_back(i); }

    REQUIRE(std::distance(arr.begin(), arr.end()) == 16);
    REQUIRE(std::accumulate(arr.begin(), arr.end(), 0) == 120);
  }

  SECTION("grow policy") {
    REQUIRE(fnd::index_array<int>::grow_factor() == 3);

    REQUIRE(fnd::index_array<int>::get_grown_capacity(0) == 1);
    REQUIRE(fnd::index_array<int>::get_grown_capacity(1) == 4);
    REQUIRE(fnd::index_array<int>::get_grown_capacity(4) == 13);
    REQUIRE(fnd::index_array<int>::get_grown_capacity(13) == 40);
  }
}
