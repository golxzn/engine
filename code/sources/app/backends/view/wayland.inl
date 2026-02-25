#include <cstring>

#include <wayland-client.h>
#include <xdg-shell.h>

#if defined(GZN_GFX_BACKEND_VULKAN)
#  include <vulkan/vulkan.h>
#  include <vulkan/vulkan_wayland.h>
#endif // defined(GZN_GFX_BACKEND_VULKAN)

#include "gzn/app/view.hpp"
#include "gzn/gfx/backends/ctx/vulkan.hpp"
#include "gzn/gfx/context.hpp"

namespace gzn::app::backends {

struct backend_context {
  backend_id   identifier{};
  glm::u32vec2 size{};

  struct {
    wl_display    *display{ nullptr };
    wl_registry   *registry{ nullptr };
    wl_compositor *compositor{ nullptr };
    wl_surface    *surface{ nullptr };
    xdg_wm_base   *shell{ nullptr };
    xdg_surface   *shell_surface{ nullptr };
    xdg_toplevel  *top_level{ nullptr };
  } wl;

  struct {
    bool was_resized : 1 { true };
    bool is_closing  : 1 { false };
  } flags;

#if defined(GZN_GFX_BACKEND_VULKAN)
  static constexpr std::array required_extensions{
    "VK_KHR_wayland_surface",
  };

  struct {
    VkSurfaceKHR surface{ VK_NULL_HANDLE };
  } vk;
#endif // defined(GZN_GFX_BACKEND_VULKAN)
};

struct view {
  backend_context inline static ctx{};

  static auto construct(view_info const &info) -> backend_id {
    if (ctx.identifier != 0) { return invalid_backend_id; }

    ctx = {
      .identifier = static_cast<backend_id>(reinterpret_cast<uintptr_t>(&ctx)),
      .size       = info.size,
      .wl{},
      .flags{},
      .vk{}
    };

    ctx.wl.display = wl_display_connect(std::data(info.display_name));
    if (!ctx.wl.display) { return invalid_backend_id; }

    ctx.wl.registry = wl_display_get_registry(ctx.wl.display);
    if (!ctx.wl.display) { return invalid_backend_id; }

    wl_registry_add_listener(ctx.wl.registry, &registry_listener, nullptr);
    wl_display_roundtrip(ctx.wl.display);
    if (!ctx.wl.compositor || !ctx.wl.shell) { return invalid_backend_id; }

    ctx.wl.surface = wl_compositor_create_surface(ctx.wl.compositor);
    if (!ctx.wl.surface) { return invalid_backend_id; }

    ctx.wl.shell_surface = xdg_wm_base_get_xdg_surface(
      ctx.wl.shell, ctx.wl.surface
    );
    if (!ctx.wl.shell_surface) { return invalid_backend_id; }

    ctx.wl.top_level = xdg_surface_get_toplevel(ctx.wl.shell_surface);
    xdg_toplevel_add_listener(ctx.wl.top_level, &top_level_listener, nullptr);


    xdg_toplevel_set_title(ctx.wl.top_level, std::data(info.title));
    xdg_toplevel_set_app_id(ctx.wl.top_level, std::data(info.title));

    wl_surface_commit(ctx.wl.surface);
    wl_display_roundtrip(ctx.wl.display);
    wl_surface_commit(ctx.wl.surface);

    return ctx.identifier;
  }

  static auto is_in_use(backend_id const id) noexcept -> bool {
    return id == ctx.identifier;
  }

  static auto make_surface_proxy(
    [[maybe_unused]] backend_id id,
    gfx::backend_type           backend
  ) -> gfx::surface_proxy {
    switch (backend) {
#if defined(GZN_GFX_BACKEND_VULKAN)
      case gfx::backend_type::vulkan: return make_vulkan_surface();
#endif // defined(GZN_GFX_BACKEND_VULKAN)

#if defined(GZN_GFX_BACKEND_OPENGL)
        // case gfx::backend_type::egl   : return make_egl_surface(id);
#endif // defined(GZN_GFX_BACKEND_OPENGL)


      default: break;
    }
    return {};
  }

  static auto get_required_extensions() -> std::span<cstr const> {
#if defined(GZN_GFX_BACKEND_VULKAN)
    return ctx.required_extensions;
#else
    return {};
#endif // defined(GZN_GFX_BACKEND_VULKAN)
  }

  static auto take_event(backend_id id, event &ev) noexcept -> bool {
    if (!is_in_use(id)) { return false; }

    return false;
  }

  static void destroy(backend_id const id) {
    if (!is_in_use(id)) { return; }

    xdg_toplevel_destroy(ctx.wl.top_level);
    xdg_surface_destroy(ctx.wl.shell_surface);
    wl_surface_destroy(ctx.wl.surface);
    xdg_wm_base_destroy(ctx.wl.shell);
    wl_compositor_destroy(ctx.wl.compositor);
    wl_registry_destroy(ctx.wl.registry);
    wl_display_disconnect(ctx.wl.display);
  }

  static auto get_size(backend_id id) noexcept -> glm::u32vec2 {
    return is_in_use(id) ? ctx.size : glm::u32vec2{};
  }

  static auto get_native_handle(backend_id id) noexcept -> native_view_handle {
    return is_in_use(id) ? ctx.wl.display : nullptr;
  }


private:
  static void on_registry_create(
    void *,
    wl_registry *registry,
    uint32_t     name,
    char const  *interface,
    uint32_t
  ) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
      ctx.wl.compositor = static_cast<wl_compositor *>(
        wl_registry_bind(registry, name, &wl_compositor_interface, 1)
      );
      return;
    }

    if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
      ctx.wl.shell = static_cast<xdg_wm_base *>(
        wl_registry_bind(registry, name, &xdg_wm_base_interface, 1)
      );
      xdg_wm_base_add_listener(ctx.wl.shell, &shell_listener, nullptr);
      return;
    }
  }

  static void on_registry_remove(
    void        *data,
    wl_registry *wl_registry,
    uint32_t     name
  ) {}

  wl_registry_listener inline static const registry_listener{
    .global        = on_registry_create,
    .global_remove = on_registry_remove,
  };

  static void on_shell_ping(void *, xdg_wm_base *, uint32_t serial) {
    xdg_wm_base_pong(ctx.wl.shell, serial);
  }

  xdg_wm_base_listener inline static const shell_listener{
    .ping = on_shell_ping,
  };

  static void on_top_level_configure(
    void *,
    xdg_toplevel *,
    int32_t width,
    int32_t height,
    wl_array *
  ) {
    glm::u32vec2 const size{ width, height };
    if (size != ctx.size) {
      ctx.flags.was_resized = true;
      ctx.size              = size;
    }
  }

  static void on_top_level_close(void *, xdg_toplevel *) {
    ctx.flags.is_closing = true;
  }

  xdg_toplevel_listener inline static const top_level_listener{
    .configure        = &on_top_level_configure,
    .close            = &on_top_level_close,
    .configure_bounds = nullptr,
    .wm_capabilities  = nullptr,
  };

#if defined(GZN_GFX_BACKEND_VULKAN)

  static auto make_vulkan_surface() -> gfx::surface_proxy {
    static fnd::dummy_allocator dummy{};
    return gfx::surface_proxy{
      .setup{ dummy,              &view::vk_setup },
      .destroy{ dummy,            &view::vk_destroy },
      .present{ dummy,            &view::vk_present },
      .get_size{ dummy,           &view::vk_get_size },
      .get_handle{ dummy, &view::vk_get_surface_handle },
    };
  }

  static auto vk_setup(fnd::util::unsafe_any_ref gfx_data) -> bool {
    auto vk{ gfx_data.as<gfx::backends::ctx::vulkan>() };

    VkWaylandSurfaceCreateInfoKHR const create_info{
      .sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
      .pNext   = nullptr,
      .flags   = 0,
      .display = ctx.wl.display,
      .surface = ctx.wl.surface,
    };
    auto const result{ vkCreateWaylandSurfaceKHR(
      vk->instance, &create_info, vk->allocator, &ctx.vk.surface
    ) };
    return VK_SUCCESS == result;
  }

  static void vk_destroy(fnd::util::unsafe_any_ref gfx_data) {
    auto vk{ gfx_data.as<gfx::backends::ctx::vulkan>() };
    vkDestroySurfaceKHR(vk->instance, ctx.vk.surface, vk->allocator);
    ctx.vk.surface = VK_NULL_HANDLE;
  }

  static void vk_present(fnd::util::unsafe_any_ref) {
    wl_display_roundtrip(ctx.wl.display);
  }

  static auto vk_get_size() noexcept -> glm::u32vec2 { return ctx.size; }

  static auto vk_get_surface_handle() noexcept -> gfx::surface_handle {
    return ctx.vk.surface;
  }

#endif // defined(GZN_GFX_BACKEND_VULKAN)
};

} // namespace gzn::app::backends
