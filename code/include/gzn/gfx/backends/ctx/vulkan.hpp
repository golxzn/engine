#pragma once

#if defined(GZN_GFX_BACKEND_VULKAN)

#  include <array>
#  include <tuple>

#  include <glad/vulkan.h>
#  include <vulkan/vulkan.h>

#  include "gzn/fnd/containers/pool.hpp"
#  include "gzn/fnd/containers/span.hpp"
#  include "gzn/fnd/log-func.hpp"
#  include "gzn/fnd/util/unsafe-any-ref.hpp"
#  include "gzn/gfx/render-capacities.hpp"
#  include "gzn/gfx/surface.hpp"
#  include "gzn/gfx/swap-chain.hpp"

namespace gzn::gfx {
struct context_info;
} // namespace gzn::gfx

namespace gzn::gfx::backends::ctx {

struct vk_memory {
  VkDeviceMemory handle{ VK_NULL_HANDLE };
  VkDevice       device{ VK_NULL_HANDLE };
};

struct vk_swapchain_element {
  VkCommandBuffer cmd_buffer{ VK_NULL_HANDLE };
  VkImage         image{ VK_NULL_HANDLE };
  VkImageView     image_view{ VK_NULL_HANDLE };
  VkFramebuffer   framebuffer{ VK_NULL_HANDLE };
  VkSemaphore     start_semaphore{ VK_NULL_HANDLE };
  VkSemaphore     end_semaphore{ VK_NULL_HANDLE };
  VkFence         fence{ VK_NULL_HANDLE };
  VkFence         last_fence{ VK_NULL_HANDLE };
};

inline constexpr u32 max_swapchain_elements{ 8 };
using swapchain_elements_array =
  std::array<vk_swapchain_element, max_swapchain_elements>;

struct vk_swapchain {
  VkSwapchainKHR           handle{ VK_NULL_HANDLE };
  u32                      images_count{};
  VkFormat                 format{};
  VkExtent2D               extent{};
  swapchain_elements_array images{};
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
  fnd::log_func log{};

  VkSurfaceKHR     surface{ VK_NULL_HANDLE };
  VkPhysicalDevice physical_device{ VK_NULL_HANDLE };
  VkDevice         logical_device{ VK_NULL_HANDLE };
  VkQueue          queue{ VK_NULL_HANDLE };
  VkCommandPool    command_pool{ VK_NULL_HANDLE };

  fnd::span<byte>        storage{};
  fnd::pool<vk_pipeline> pipelines;
  fnd::pool<vk_buffer>   buffers;
  fnd::pool<vk_sampler>  samplers;

  static void load();
  static void unload();

  static auto is_available() -> bool;

  static auto calc_required_space_for(render_capacities const &caps) noexcept
    -> usize;

  static auto make_context_on(
    context_info const       &info,
    fnd::util::unsafe_any_ref extra
  ) -> vulkan *;

  static auto setup(
    fnd::span<byte>     storage,
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

  static auto create_swapchain(
    VkAllocationCallbacks *alloc,
    fnd::log_func         &log,
    swapchain_info const  &info,
    VkSurfaceKHR           surface,
    VkPhysicalDevice       physical_device,
    VkDevice               logical_device
  ) -> vk_swapchain;
};

} // namespace gzn::gfx::backends::ctx

#endif // defined(GZN_GFX_BACKEND_VULKAN)
