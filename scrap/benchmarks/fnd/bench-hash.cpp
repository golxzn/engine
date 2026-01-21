#include <concepts>
#include <vector>

#include <gzn/fnd/hash.hpp>
#include <nanobench.h>

#include "common/genstr.hpp"

#define RAPIDHASH_UNROLLED
#include "rapidhash.hpp"

template<class T>
std::string_view inline constexpr char_type_name{ typeid(T).name() };

template<>
std::string_view inline constexpr char_type_name<char>{ "char" };
template<>
std::string_view inline constexpr char_type_name<char8_t>{ "char8_t" };
template<>
std::string_view inline constexpr char_type_name<char16_t>{ "char16_t" };
template<>
std::string_view inline constexpr char_type_name<char32_t>{ "char32_t" };

template<class T, size_t L>
auto run_hash_bench(ankerl::nanobench::Bench &bench) {
  auto const title{
    std::format("gzn::fnd::hash<{:8}>({:4})", char_type_name<T>, L)
  };
  return bench.run(title, [str{ cmn::genstr<T, L>() }] {
    ankerl::nanobench::doNotOptimizeAway(
      gzn::fnd::hash<T>({
        .key{ std::data(str), std::size(str) }
    })
    );
  });
}

int main() {
  using namespace ankerl;

  nanobench::Bench bench{};
  bench.run("std::hash<sv>{}(   2)", [str{ cmn::genstr<char>(2) }] {
    nanobench::doNotOptimizeAway(std::hash<std::string_view>{}(str));
  });
  bench.run("std::hash<sv>{}(   4)", [str{ cmn::genstr<char>(4) }] {
    nanobench::doNotOptimizeAway(std::hash<std::string_view>{}(str));
  });
  bench.run("std::hash<sv>{}(  16)", [str{ cmn::genstr<char>(16) }] {
    nanobench::doNotOptimizeAway(std::hash<std::string_view>{}(str));
  });
  bench.run("std::hash<sv>{}(  32)", [str{ cmn::genstr<char>(32) }] {
    nanobench::doNotOptimizeAway(std::hash<std::string_view>{}(str));
  });
  bench.run("std::hash<sv>{}( 100)", [str{ cmn::genstr<char>(100) }] {
    nanobench::doNotOptimizeAway(std::hash<std::string_view>{}(str));
  });
  bench.run("std::hash<sv>{}(1000)", [str{ cmn::genstr<char>(1000) }] {
    nanobench::doNotOptimizeAway(std::hash<std::string_view>{}(str));
  });


  bench.run("rapidhash(   2)", [str{ cmn::genstr<char>(2) }] {
    nanobench::doNotOptimizeAway(rapidhash(std::data(str), std::size(str)));
  });
  bench.run("rapidhash(   4)", [str{ cmn::genstr<char>(4) }] {
    nanobench::doNotOptimizeAway(rapidhash(std::data(str), std::size(str)));
  });
  bench.run("rapidhash(  16)", [str{ cmn::genstr<char>(16) }] {
    nanobench::doNotOptimizeAway(rapidhash(std::data(str), std::size(str)));
  });
  bench.run("rapidhash(  32)", [str{ cmn::genstr<char>(32) }] {
    nanobench::doNotOptimizeAway(rapidhash(std::data(str), std::size(str)));
  });
  bench.run("rapidhash( 100)", [str{ cmn::genstr<char>(100) }] {
    nanobench::doNotOptimizeAway(rapidhash(std::data(str), std::size(str)));
  });
  bench.run("rapidhash(1000)", [str{ cmn::genstr<char>(1000) }] {
    nanobench::doNotOptimizeAway(rapidhash(std::data(str), std::size(str)));
  });

  run_hash_bench<char, 2>(bench);
  run_hash_bench<char, 4>(bench);
  run_hash_bench<char, 16>(bench);
  run_hash_bench<char, 32>(bench);
  run_hash_bench<char, 100>(bench);
  run_hash_bench<char, 1000>(bench);

  run_hash_bench<char16_t, 2>(bench);
  run_hash_bench<char16_t, 4>(bench);
  run_hash_bench<char16_t, 16>(bench);
  run_hash_bench<char16_t, 32>(bench);
  run_hash_bench<char16_t, 100>(bench);
  run_hash_bench<char16_t, 1000>(bench);

  run_hash_bench<char32_t, 2>(bench);
  run_hash_bench<char32_t, 4>(bench);
  run_hash_bench<char32_t, 16>(bench);
  run_hash_bench<char32_t, 32>(bench);
  run_hash_bench<char32_t, 100>(bench);
  run_hash_bench<char32_t, 1000>(bench);
}
