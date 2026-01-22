#include <gzn/application>
#include <gzn/foundation>
#include <gzn/graphics>

int main() {
  using namespace gzn;
  fnd::base_allocator alloc{};

  auto view{
    app::view::make<fnd::stack_owner>(alloc, { .size{ 1920, 1080 } })
  };
  if (!view.is_alive()) { return EXIT_FAILURE; }

  auto render{ gfx::render::make<fnd::stack_owner>(
    alloc, { .backend = gfx::backend_type::opengl_es }
  ) };
  if (!render.is_alive()) { return EXIT_FAILURE; }
  fnd::ref render_ref{ render };

  gfx::text_draw_info const hello_world{
    .text{ "Hello, World!" },
    .pos{ 100.f, 100.f },
    .color = 0xFD'9D'66'FF
  };

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

    gfx::cmd::start(render_ref, 0x27'27'2E'FF);
    gfx::cmd::draw(render_ref, hello_world);
    gfx::cmd::submit(render_ref);

    view->swap_buffers();
  }
}
