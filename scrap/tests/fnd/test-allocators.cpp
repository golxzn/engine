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

static void base_allocator_check(
  gzn::fnd::util::allocator_type auto &alloc,
  gzn::usize const                     vertices_count
) {
  using namespace gzn;

  auto block{ alloc.allocate(fnd::util::size_of<vertex>(vertices_count)) };
  REQUIRE(block);
  REQUIRE(alloc.owns(block));
  REQUIRE_FALSE(alloc.owns({}));

  auto test_vertex{ new (block.address) vertex{} };

  alloc.deallocate(block);
  REQUIRE_FALSE(alloc.owns(block));
}

TEST_CASE("test: gzn::fnd::allocators", "[fnd][allocators]") {
  using namespace gzn;

  SECTION("dummy_allocator") {

    fnd::dummy_allocator dummy{};
    REQUIRE(dummy.owns({}));

  } // SECTION("dummy_allocator")

  SECTION("heap_allocator") {

    fnd::heap_allocator heap{};
    REQUIRE(heap.is_valid());
    base_allocator_check(heap, 10);

  } // SECTION("base_allocator")

  // SECTION("mmap-allocator") {
  // } // SECTION("mmap-allocator")

  SECTION("in-stack-allocator") {

    fnd::in_stack_allocator<2_KiB> stack{};
    base_allocator_check(stack, 10);

    auto const size{ fnd::util::size_of<vertex>() };
    auto block_0{ stack.allocate(size) };
    auto block_1{ stack.allocate(size) };

    stack.deallocate(block_1);
    stack.deallocate(block_0);

  } // SECTION("in-stack-allocator")

  SECTION("fallback-allocator") {
    using alloc = fnd::
      fallback_allocator<fnd::in_stack_allocator<2_KiB>, fnd::heap_allocator>;

    alloc fb{};
    base_allocator_check(fb, 10);
    base_allocator_check(fb, 10000);

  } // SECTION("in-stack-allocator")

  SECTION("affix-allocator") {
    static usize IDX{};

    struct _prefix {
      usize index;

      _prefix(fnd::memory_block_size size, std::source_location loc)
        : index{ ++IDX } {
        SUCCEED(
          "PREFIX [" << index << "] Allocated in block (" << size.bytes_count
                     << ", " << size.alignment << ") in " << loc.file_name()
                     << "@" << loc.line() << ":" << loc.column() << " at "
                     << loc.function_name()
        );
      }
    };

    struct _postfix {
      usize index;

      _postfix(fnd::memory_block_size size, std::source_location loc)
        : index{ ++IDX } {
        SUCCEED(
          "POSTFIX [" << index << "] Allocated in block (" << size.bytes_count
                      << ", " << size.alignment << ") in " << loc.file_name()
                      << "@" << loc.line() << ":" << loc.column() << " at "
                      << loc.function_name()
        );
      }
    };

    fnd::affix_allocator<fnd::heap_allocator, _prefix> prefix_only{};
    base_allocator_check(prefix_only, 2);

    fnd::affix_allocator<fnd::heap_allocator, _prefix, _postfix>
      prefix_postfix{};
    base_allocator_check(prefix_postfix, 3);

  } // SECTION("in-stack-allocator")


} // TEST_CASE("common", "[raw-data]")
