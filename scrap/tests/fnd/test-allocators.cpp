#include <catch2/catch_test_macros.hpp>
#include <gzn/fnd/allocators.hpp>

struct float2 {
  float x{}, y{};
};

struct float3 : float2 {
  float z{};
};

struct float4 : float3 {
  float w{};
};

#pragma push(pack, 1)

struct vertex {
  float3 pos{};
  float4 clr{};
  float2 uvs{};
};

#pragma pop(pack)

TEST_CASE("test: gzn::fnd::allocators", "[fnd][allocators]") {

  SECTION("dummy_allocator") {
    gzn::fnd::dummy_allocator dummy{};
    REQUIRE(dummy.allocate(100, 0) == nullptr);
    REQUIRE(dummy.allocate(100, 0, 0, 0) == nullptr);
  } // SECTION("dummy_allocator")

  SECTION("base_allocator") {
    gzn::fnd::base_allocator base{ "test-alloc" };
    constexpr size_t         bytes_count{ 1024 };

    void *mem{ base.allocate(bytes_count) };
    REQUIRE(mem != nullptr);

    auto test_vertex{ new (mem) vertex{} };

    base.deallocate(mem, bytes_count);
  } // SECTION("base_allocator")

} // TEST_CASE("common", "[raw-data]")
