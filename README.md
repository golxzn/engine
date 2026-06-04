
<div align="center">

# gzn::engine

> \[over-engineered, hard-to-use, ugly game engine to enhance absurd]

![C++23][cpp-badge]
![CMake][cmake-badge]

</div>

## How to use

```cmake
add_subdirectory(gzn-engine)
target_link_libraries(your-target PRIVATE gzn::engine)
```

## `/High Level/` No time to deal with complex rendering

```cpp
#include <gzn/foundation>
#include <gzn/graphics>
#include <gzn/application>

int main() {
  using namespace gzn;
  fnd::base_allocator alloc{};

  auto view{ app::view::make<fnd::stack_owner>(alloc, {
    .title{ "Triangle test" },
    .size{ 1940u, 1080u }
  }) };
  if (!view.is_alive()) { return EXIT_FAILURE; }

  auto constexpr gfx_backend{ gfx::context::select_available(
    gfx::backend_type::vulkan
  ) };
  auto ctx{ gfx::context::make(gfx_storage, {
    .backend = gfx_backend,
    .surface{ view->make_surface_proxy(gfx_backend) },
  }) };
  if (!ctx.is_valid()) {
    return EXIT_FAILURE;
  }
  gfx::cmd::init(ctx);

  auto hl{ gfx::high_level::make(ctx) };

  auto const hello_world{ hl->bake(gfx::hl::draw_info_text{
    .text{ "Hello, World!" },
    .pos{ view->get_size() / 2 },
    .color = 0xFD'9D'66'FF,
  }) };

  app::event event{};
  for (bool running{ true }; running;) {
    while (view->take_next_event(event)) {
      if (auto key{ std::get_if<app::event_key_released>(&event) }; key) {
        if (fnd::util::any_from(key->code, app::key::q, app::key::escape)) {
          running = false;
          break;
        }
      }
    }

    gfx::cmd::begin_frame(ctx);
    gfx::cmd::submit(hello_world);
    gfx::cmd::end_frame(ctx);
  }
}
```

## `/Low  Level/` Nah. I'm a professional

It's too complex and big. I afraid it will be scary to see something like that
in the main readme file. So, welcome to the [examples directory][examples-dir].


[cpp-badge]: https://img.shields.io/badge/C%2B%2B23-%2300599C?style=flat&logo=cplusplus
[cmake-badge]: https://img.shields.io/badge/CMake-%23064F8C?style=flat&logo=cmake

[examples-dir]: https://github.com/golxzn/engine/tree/main/scrap/examples/readme.md

