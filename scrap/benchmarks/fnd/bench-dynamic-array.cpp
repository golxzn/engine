#include <vector>

#include <gzn/fnd/containers/dynamic-array.hpp>
#include <mimalloc.h>
#include <nanobench.h>

int main() {
  using namespace gzn;
  using namespace ankerl;

  struct grow_info {
    size_t size{}, capacity{};
  };

  static std::array<size_t, 100> pushing_values{};
  std::generate_n(std::begin(pushing_values), std::size(pushing_values), [] {
    return static_cast<size_t>(std::rand() % 100);
  });

  nanobench::Bench bench{};

  bench.run("std::vector grow", [] {
    std::vector<size_t> growing;
    for (auto const value : pushing_values) { growing.push_back(value); }
    return growing.size() + growing.capacity();
  });

  bench.run("std::vector grow (mi_stl_allocator)", [] {
    std::vector<size_t, mi_stl_allocator<size_t>> growing;
    for (auto const value : pushing_values) { growing.push_back(value); }
    return growing.size() + growing.capacity();
  });

  bench.run("dynamic_array grow", [alloc{ fnd::base_allocator{} }]() mutable {
    fnd::dynamic_array<size_t> growing{ alloc };
    for (auto const value : pushing_values) { growing.push_back(value); }
    return growing.size() + growing.capacity();
  });
}
