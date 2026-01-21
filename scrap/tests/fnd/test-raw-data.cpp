#include <catch2/catch_test_macros.hpp>
#include <gzn/fnd/raw-data.hpp>

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

TEST_CASE("test: gzn::raw_data", "[fnd][raw-data]") {
  gzn::fnd::raw_data const value0{ {
    vertex{ { -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
    vertex{   { 0.0f, 0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.5f, 1.0f } },
    vertex{  { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
  } };
  gzn::fnd::raw_data const value1{ {
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, //
    0.0f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f, //
    0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f
  } };
  gzn::fnd::raw_data const value2{ { -0.5, -0.5, 0.0, 0.0, 1.0, 1.0, 0.0 } };

  SECTION("size/stride") {
    REQUIRE(std::size(value0) == sizeof(vertex) * 3);
    REQUIRE(std::size(value1) == sizeof(vertex) * 3);
    REQUIRE(std::size(value2) == sizeof(double) * 7);

    REQUIRE(value0.stride() == sizeof(vertex));
    REQUIRE(value1.stride() == sizeof(float));
    REQUIRE(value2.stride() == sizeof(double));
  } // SECTION("size/stride")

  SECTION("is_same_as/is_similar_to") {
    REQUIRE(value0.is_same_as(value0));
    REQUIRE(value0.is_similar_to(value0));

    REQUIRE_FALSE(value0.is_same_as(value1));
    REQUIRE_FALSE(value0.is_same_as(value2));

    REQUIRE(value0.is_similar_to(value1));
    REQUIRE_FALSE(value0.is_similar_to(value2));
  } // SECTION("is_same_as/is_similar_to")

} // TEST_CASE("common", "[raw-data]")
