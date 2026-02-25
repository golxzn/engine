#if defined(GZN_GFX_BACKEND_VULKAN)

#  include <tuple>

#  include <vulkan/vulkan.hpp>

#  include "gzn/fnd/containers/pool.hpp"
#  include "gzn/fnd/util/unsafe_any_ref.hpp"
#  include "gzn/gfx/render-capacities.hpp"
#  include "gzn/gfx/surface.hpp"

namespace gzn::gfx {
struct context_info;
} // namespace gzn::gfx

namespace gzn::gfx::backends::ctx {

struct vk_memory {
  VkDeviceMemory handle{ VK_NULL_HANDLE };
  VkDevice       device{ VK_NULL_HANDLE };
};

struct vk_pipeline {
  VkPipeline handle{ VK_NULL_HANDLE };
};

struct vk_buffer {
  VkBuffer  handle{ VK_NULL_HANDLE };
  vk_memory memory{};
};

struct vk_sampler {
  VkSampler handle{ VK_NULL_HANDLE };
};

struct vulkan_extra_data {
  VkAllocationCallbacks *allocator{};
};

struct vulkan {
  VkAllocationCallbacks *allocator{};
  VkInstance             instance{ VK_NULL_HANDLE };
  gzn_if_debug(VkDebugUtilsMessengerEXT debug_messenger{ VK_NULL_HANDLE });

  VkSurfaceKHR     surface{ VK_NULL_HANDLE };
  VkPhysicalDevice physical_device{ VK_NULL_HANDLE };
  VkDevice         logical_device{ VK_NULL_HANDLE };
  VkQueue          queue{ VK_NULL_HANDLE };
  VkCommandPool    command_pool{ VK_NULL_HANDLE };

  std::span<byte>                   storage{};
  fnd::non_owning_pool<vk_pipeline> pipelines;
  fnd::non_owning_pool<vk_buffer>   buffers;
  fnd::non_owning_pool<vk_sampler>  samplers;

  static auto is_available() -> bool;

  static auto calc_required_space_for(render_capacities const &caps) noexcept
    -> usize;

  static auto make_context_on(
    context_info const       &info,
    fnd::util::unsafe_any_ref extra
  ) -> vulkan *;

  static auto setup(
    std::span<byte>     storage,
    context_info const &info,
    surface_proxy      &surface
  ) -> bool;

  static void destroy();

private:
  static auto make_instance(
    VkAllocationCallbacks *alloc,
    context_info const    &info
  ) -> VkInstance;

#  if defined(GZN_DEBUG)

  static auto make_debug_messenger(
    VkAllocationCallbacks *alloc,
    VkInstance             instance
  ) -> VkDebugUtilsMessengerEXT;

#  endif // defined(GZN_DEBUG)

  static auto select_physical_device(
    VkInstance          instance,
    context_info const &info
  ) -> VkPhysicalDevice;

  static auto select_logical_device(
    VkAllocationCallbacks *alloc,
    VkPhysicalDevice       physical_device,
    VkSurfaceKHR           surface
  ) -> std::tuple<VkDevice, u32>;


  static auto select_device_queue(u32 queue_index, VkDevice logical_device)
    -> VkQueue;

  static auto create_command_pool(
    VkAllocationCallbacks *alloc,
    u32                    family_index,
    VkDevice               logical_device
  ) -> VkCommandPool;
};

} // namespace gzn::gfx::backends::ctx

#endif // defined(GZN_GFX_BACKEND_VULKAN)
