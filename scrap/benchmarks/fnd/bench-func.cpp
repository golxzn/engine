#include <functional>

#include <gzn/fnd/func.hpp>
#include <nanobench.h>

struct payload {
  gzn::c_array<gzn::u64, 25> u64arr100{};
  gzn::u8                    unaligned{ 10 };
  gzn::u32                   uninitialized;
};

template<class T>
class func_test {
public:
  explicit func_test(T val) noexcept
    : value{ std::move(val) } {}

  auto testing(int value) -> int { return value; }

private:
  T value;
};

auto small_function(int x) -> int { return x; }

int main() {
  using namespace gzn;
  using namespace ankerl;

  int const param{ std::rand() };

  nanobench::Bench                bench{};
  static gzn::fnd::base_allocator alloc{};

  bench.title("functor creation");
  bench.run("[gzn] small function creation", [] {
    return fnd::move_only_func<int(int)>{ alloc, small_function };
  });

  bench.run("[std] small function creation", [] {
    return std::function<int(int)>{ small_function };
  });

  bench.run("[gzn] small lambda creation", [] {
    return fnd::move_only_func<int(int)>{ alloc, [](int x) { return x; } };
  });

  bench.run("[std] small lambda creation", [] {
    return std::function<int(int)>{ [](int x) { return x; } };
  });

  func_test<payload> heavy_dude{ payload{} };
  bench.run("[gzn] huge lambda creation", [heavy_dude] {
    return fnd::move_only_func<int(int)>{ alloc,
                                          [heavy_dude](int x) { return x; } };
  });

  bench.run("[std] huge lambda creation", [heavy_dude] {
    return std::function<int(int)>{ [heavy_dude](int x) { return x; } };
  });


  bench.title("function calling");
  bench.run(
    "[gzn] lambda | no capture",
    [param,
     fn{
       fnd::move_only_func<int(int)>{
                                     alloc, [](int x) { return x; } }
  }]() mutable { return fn(param); }
  );

  bench.run(
    "[std] lambda | no capture",
    [param, fn{ std::function<int(int)>{ [](int x) {
       return x;
     } } }]() mutable { return fn(param); }
  );

  bench.run(
    "[gzn] lambda | huge capture",
    [param,
     fn{
       fnd::move_only_func<int(int)>{ alloc, [heavy_dude](int x) {
                                       return x;
                                     } }
  }]() mutable { return fn(param); }
  );

  bench.run(
    "[std] lambda | huge capture",
    [param, fn{ std::function<int(int)>{ [heavy_dude](int x) {
       return x;
     } } }]() mutable { return fn(param); }
  );

  func_test obj{ param };
  using method_signature = int(func_test<int> &, int);
  bench.run(
    "[gzn] method",
    [fn{
       fnd::move_only_func<method_signature>{ alloc,
                                             &func_test<int>::testing }
  },
     obj,
     param]() mutable { return std::invoke_r<int>(fn, obj, param); }
  );

  bench.run(
    "[std] method",
    [fn{ std::function<method_signature>{ &func_test<int>::testing } },
     obj,
     param]() mutable { return std::invoke_r<int>(fn, obj, param); }
  );
}
