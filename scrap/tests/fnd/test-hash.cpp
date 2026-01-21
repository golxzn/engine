#include <memory_resource>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <gzn/fnd/allocators.hpp>
#include <gzn/fnd/hash.hpp>

#include "common/genstr.hpp"

gzn::usize constexpr JOBS{ 30 };
gzn::usize constexpr ELEMENT_LENGTH{ 6zu };
gzn::usize constexpr ELEMENTS_COUNT{ 10'000zu };

gzn_inline auto generate_elements(std::pmr::polymorphic_allocator<char> &alloc)
  -> std::pmr::vector<std::pmr::string> {
  std::pmr::vector<std::pmr::string> elements{ alloc };
  elements.reserve(ELEMENTS_COUNT);

  // This is oh my shit so fucking slow. Anyway, it's tests who cares (me)
  while (std::size(elements) != ELEMENTS_COUNT) {
    auto const elem{ cmn::genstr<char, ELEMENT_LENGTH>() };
    if (!std::ranges::contains(elements, elem)) {
      elements.emplace_back(elem);
    }
  }

  return elements;
}

void calculate_enthropy(
  std::span<std::pmr::string const> elements,
  std::span<gzn::u64>               out
) {
  for (gzn::usize i{}; i < std::size(elements); ++i) {
    auto const &element{ elements[i] };
    out[i] = gzn::fnd::hash<char>({
      .key{ std::data(element), std::size(element) }
    });
  }
}

auto calculate_enthropy_async(std::span<std::pmr::string const> elements)
  -> gzn::usize {
  auto const elem_count{ std::size(elements) };
  auto const elem_per_job{ elem_count / JOBS };
  auto const remain_elem_count{ elem_count - (elem_per_job * JOBS) };

  std::array<std::jthread, JOBS> jobs{};
  std::vector<gzn::u64>          hashes(elem_count, gzn::u64{});
  std::span<gzn::u64> hashes_span{ std::data(hashes), std::size(hashes) };

  for (gzn::usize i{}, offset{}; i < JOBS; ++i, offset += elem_per_job) {
    jobs[i] = std::jthread{ &calculate_enthropy,
                            elements.subspan(offset, elem_per_job),
                            hashes_span.subspan(offset, elem_per_job) };
  }

  if (remain_elem_count != 0) {
    calculate_enthropy(
      elements.subspan(elem_per_job * JOBS, remain_elem_count),
      hashes_span.subspan(elem_per_job * JOBS, remain_elem_count)
    );
  }

  std::ranges::for_each(jobs, &std::jthread::join);
  std::ranges::sort(hashes);

  gzn::usize collisions{};
  for (gzn::usize i{ 1zu }; i < elem_count; ++i) {
    collisions += static_cast<gzn::usize>(hashes[i] == hashes[i - 1]);
  }
  return collisions;
}

TEST_CASE("test: gzn::fnd::hash", "[fnd][hash]") {
  using namespace gzn;
  gzn::usize constexpr STORAGE_SIZE{ ELEMENTS_COUNT * (ELEMENT_LENGTH + 1) };

  std::pmr::monotonic_buffer_resource   resource{ STORAGE_SIZE };
  std::pmr::polymorphic_allocator<char> alloc{ &resource };

  auto const elements{ generate_elements(alloc) };
  auto const enthropy{ calculate_enthropy_async(elements) };
  SUCCEED("Enthropy of " << std::size(elements) << " is " << enthropy);
  REQUIRE(std::size(elements) / 8zu >= enthropy);

} // TEST_CASE("common", "[raw-data]")
