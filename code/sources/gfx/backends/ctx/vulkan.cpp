#include "gzn/gfx/backends/ctx/vulkan.hpp"

#include <algorithm>

#include "gzn/fnd/containers/dynamic-array.hpp"
#include "gzn/fnd/util/unsafe_any_ref.hpp"
#include "gzn/gfx/context.hpp"
#include "gzn/gfx/gpu-info.hpp"

namespace gzn::gfx::backends::ctx {

#define GET_EXTENSION_FUNCTION(_id)                    \
  ((PFN_##_id)(vkGetInstanceProcAddr(instance, #_id)))

namespace {

vulkan g_ctx{};

VkAllocationCallbacks g_default_callbacks{
  .pUserData             = nullptr,
  .pfnAllocation         = nullptr,
  .pfnReallocation       = nullptr,
  .pfnFree               = nullptr,
  .pfnInternalAllocation = nullptr,
  .pfnInternalFree       = nullptr,
};

static auto on_vulkan_error(
  VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
  VkDebugUtilsMessageTypeFlagsEXT             type,
  VkDebugUtilsMessengerCallbackDataEXT const *callback_data,
  void                                       *user_data
) -> VkBool32 {
  std::printf("[Vulkan]");

  switch (type) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
      std::printf("[general    ]");
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
      std::printf("[validation ]");
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
      std::printf("[performance]");
      break;
  }

  switch (severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      std::printf("[verbose] ");
      break;
    default:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      std::printf("[info   ] ");
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      std::printf("[warning] ");
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      std::printf("[error  ] ");
      break;
  }

  std::printf("%s\n", callback_data->pMessage);
  return 0;
}

} // namespace

struct struct_sizes {
  static constexpr auto pipeline_bytes_count{ sizeof(vk_pipeline) };
  static constexpr auto buffer_bytes_count{ sizeof(vk_buffer) };
};

struct gen_counter_size {
  static constexpr auto pipeline_bytes_count{ sizeof(fnd::gen_counter) };
  static constexpr auto buffer_bytes_count{ sizeof(fnd::gen_counter) };
};

struct offset_accumulator {
  std::span<byte> iter;

  template<class T>
  constexpr auto set(usize const count) noexcept {
    auto place{ iter.subspan(0, count) };
    iter = iter.subspan(count);
    return place;
  }
};

auto vulkan::is_available() -> bool {
  u32        ext_count;
  auto const status{
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr)
  };
  return status == VK_SUCCESS && ext_count != 0;
}

auto vulkan::calc_required_space_for(render_capacities const &caps) noexcept
  -> usize {
  return caps.total_size<struct_sizes>() + caps.total_size<gen_counter_size>();
}

auto vulkan::make_context_on(
  context_info const       &info,
  fnd::util::unsafe_any_ref extra_storage
) -> vulkan * {
  if (g_ctx.instance != VK_NULL_HANDLE) {
    /// @todo think: nullptr or existing instance?
    /// @todo error log
    return nullptr;
  }

  auto const extra{ extra_storage.as<vulkan_extra_data>() };
  auto alloc{ extra ? extra->allocator : /*&g_default_callbacks*/ nullptr };

  auto instance{ make_instance(alloc, info) };
  if (instance == VK_NULL_HANDLE) { return nullptr; }

#if defined(GZN_DEBUG)
  auto debug_messenger{ make_debug_messenger(alloc, instance) };
#endif // defined(GZN_DEBUG)

  g_ctx = vulkan{
    .allocator = alloc,
    .instance  = instance,
#if defined(GZN_DEBUG)
    .debug_messenger = debug_messenger,
#endif // defined(GZN_DEBUG)
  }; // NOLINT
  return &g_ctx;
}

auto vulkan::setup(
  std::span<byte>     storage,
  context_info const &info,
  surface_proxy      &surface_wrapper
) -> bool {
  if (g_ctx.instance == VK_NULL_HANDLE) { return false; }

  auto const &caps{ info.capacities };
  auto const  required_space{ calc_required_space_for(caps) };
  if (std::size(storage) != required_space) {
    // @todo error log
    return false;
  }

  auto alloc{ g_ctx.allocator };
  auto surface{ static_cast<VkSurfaceKHR>(surface_wrapper.get_handle()) };

  auto physical_device{ select_physical_device(g_ctx.instance, info) };
  if (physical_device == VK_NULL_HANDLE) {
    // @todo error log
    return false;
  }

  auto [logical_device, queue_index]{
    select_logical_device(alloc, physical_device, surface)
  };
  auto queue{ select_device_queue(queue_index, logical_device) };
  auto command_pool{ create_command_pool(alloc, queue_index, logical_device) };


  offset_accumulator off{ .iter{ storage } };
  auto const offset_pipelines{ off.set<vk_pipeline>(caps.pipelines_count) };
  auto const offset_buffers{ off.set<vk_buffer>(caps.buffers_count) };
  auto const offset_samplers{ off.set<vk_sampler>(caps.samples_count) };

  g_ctx = vulkan{
    .allocator = g_ctx.allocator,
    .instance  = g_ctx.instance,
#if defined(GZN_DEBUG)
    .debug_messenger = g_ctx.debug_messenger,
#endif  // defined(GZN_DEBUG)

    .surface         = surface,
    .physical_device = physical_device,
    .logical_device  = logical_device,
    .queue           = queue,
    .command_pool    = command_pool,

    .storage{ storage },
    .pipelines{ std::data(offset_pipelines), caps.pipelines_count },
    .buffers{ std::data(offset_buffers), caps.buffers_count },
    .samplers{ std::data(offset_samplers), caps.samples_count },
  };
  return true;
}

void vulkan::destroy() {
  vkDestroyDevice(g_ctx.logical_device, g_ctx.allocator);

#if defined(GZN_DEBUG)
  {
    auto instance{ g_ctx.instance };
    GET_EXTENSION_FUNCTION(vkDestroyDebugUtilsMessengerEXT)(
      instance, g_ctx.debug_messenger, g_ctx.allocator
    );
  }
#endif // defined(GZN_DEBUG)

  vkDestroyInstance(g_ctx.instance, g_ctx.allocator);

  g_ctx = {};
}

// ================================ PRIVATE ================================ //

static auto count_matching_layers(auto const &required_layers) -> usize {
  uint32_t device_layer_count;
  VkResult result{
    vkEnumerateInstanceLayerProperties(&device_layer_count, nullptr)
  };
  if (result != VK_SUCCESS) { return 0; }


  static fnd::stack_arena_allocator<sizeof(VkLayerProperties) * 25> tmp{};
  fnd::dynamic_array<VkLayerProperties, decltype(tmp)> layer_properties{
    tmp, static_cast<usize>(device_layer_count)
  };

  result = vkEnumerateInstanceLayerProperties(
    &device_layer_count, std::data(layer_properties)
  );
  if (result != VK_SUCCESS) { return 0; }

  usize matching_count{};
  for (auto const &layer : layer_properties) {
    for (auto const required : required_layers) {
      matching_count += static_cast<usize>(
        std::strcmp(required, layer.layerName) == 0
      );
    }
  }
  return matching_count;
}

auto vulkan::make_instance(
  VkAllocationCallbacks *alloc,
  context_info const    &info
) -> VkInstance {
  VkApplicationInfo const app_info{
    .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext              = nullptr,
    .pApplicationName   = std::data(info.app_name),
    .applicationVersion = info.app_version.value,
    .pEngineName        = std::data(info.engine_name),
    .engineVersion      = info.engine_version.value,
    .apiVersion         = VK_API_VERSION_1_0,
  };

  static constexpr usize MAX_EXTENSIONS{ 16 };

  u32                              active_ext_count{ 2 };
  std::array<cstr, MAX_EXTENSIONS> instance_extension{
    "VK_KHR_surface", gzn_if_debug("VK_EXT_debug_utils")
  };

  for (auto &&ext : info.extensions) {
    if (active_ext_count == std::size(instance_extension)) {
      /// warning
      break;
    }
    instance_extension[active_ext_count++] = ext;
  }

  std::array constexpr layer_names{ "VK_LAYER_KHRONOS_validation" };
  auto const matching_count{ count_matching_layers(layer_names) };
  auto const has_layers{ matching_count == std::size(layer_names) };

  VkInstanceCreateInfo const create_info{
    .sType               = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext               = nullptr,
    .flags               = 0,
    .pApplicationInfo    = &app_info,
    .enabledLayerCount   = static_cast<u32>(has_layers ? matching_count : 0u),
    .ppEnabledLayerNames = std::data(layer_names),
    .enabledExtensionCount   = active_ext_count,
    .ppEnabledExtensionNames = std::data(instance_extension)
  };

  VkInstance instance;
  if (VK_SUCCESS != vkCreateInstance(&create_info, alloc, &instance)) {
    // @todo error log
    return VK_NULL_HANDLE;
  }
  return instance;
}

#if defined(GZN_DEBUG)

auto vulkan::make_debug_messenger(
  VkAllocationCallbacks *alloc,
  VkInstance             instance
) -> VkDebugUtilsMessengerEXT {

  VkDebugUtilsMessengerCreateInfoEXT const msgr_create_info{
    .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .pNext           = nullptr,
    .flags           = 0,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = on_vulkan_error, /// @todo user log func
    .pUserData       = nullptr,
  };

  VkDebugUtilsMessengerEXT debug_messenger;
  GET_EXTENSION_FUNCTION(vkCreateDebugUtilsMessengerEXT)(
    instance, &msgr_create_info, alloc, &debug_messenger
  );
  return debug_messenger;
}

#endif // defined(GZN_DEBUG)

auto vulkan::select_physical_device(
  VkInstance          instance,
  context_info const &info
) -> VkPhysicalDevice {

  static constexpr u32 MAX_DEVICE_COUNT{ 16 };

  std::array<VkPhysicalDevice, MAX_DEVICE_COUNT> phys_devices;
  std::array<gpu_info, MAX_DEVICE_COUNT>         device_infos;
  u32                                            phys_devices_count{};
  vkEnumeratePhysicalDevices(instance, &phys_devices_count, nullptr);

  auto const result{ vkEnumeratePhysicalDevices(
    instance, &phys_devices_count, std::data(phys_devices)
  ) };
  if (result != VK_SUCCESS) {
    // log error
    return VK_NULL_HANDLE;
  }

  phys_devices_count = std::min(MAX_DEVICE_COUNT, phys_devices_count);

  static constexpr auto conv_type{ [](VkPhysicalDeviceType type) {
    switch (type) {
      using enum gpu_type;
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return integrated_gpu;
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU  : return discrete_gpu;
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU   : return virtual_gpu;
      case VK_PHYSICAL_DEVICE_TYPE_CPU           : return emulated_gpu;
      default                                    : break;
    }
    return gpu_type::unknown;
  } };

  for (u32 idx{}; idx < phys_devices_count; ++idx) {
    VkPhysicalDevice device = phys_devices[idx];

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    VkPhysicalDeviceMemoryProperties memory;
    vkGetPhysicalDeviceMemoryProperties(device, &memory);

    device_infos[idx] = gpu_info{
      .name{ properties.deviceName },
      .vram_bytes = memory.memoryHeaps[0].size, /// @todo not smurt enuf....
      .type       = conv_type(properties.deviceType),
    };
  }
  auto const selected_idx{ info.select_gpu(
    std::span{
      std::data(device_infos),
      static_cast<usize>(phys_devices_count),
    }
  ) };
  if (selected_idx >= phys_devices_count) {
    /// log error
    return VK_NULL_HANDLE;
  }
  return phys_devices[selected_idx];
}

auto vulkan::select_logical_device(
  VkAllocationCallbacks *alloc,
  VkPhysicalDevice       physical_device,
  VkSurfaceKHR           surface
) -> std::tuple<VkDevice, u32> {
  auto static constexpr MAX_FAMILIES{ 64 };
  std::array<VkQueueFamilyProperties, MAX_FAMILIES> queue_family_props{};
  u32                                               queue_family_props_count{};

  vkGetPhysicalDeviceQueueFamilyProperties(
    physical_device, &queue_family_props_count, std::data(queue_family_props)
  );

  u32 family_index{};
  for (u32 idx{}; idx < queue_family_props_count; ++idx) {
    VkBool32 present{ VK_FALSE };

    vkGetPhysicalDeviceSurfaceSupportKHR(
      physical_device, idx, surface, &present
    );
    if (!present) { continue; }
    if (queue_family_props[idx].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      family_index = idx;
      break;
    }
  }

  float priority = 1;

  VkDeviceQueueCreateInfo const queue_create_info{
    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .pNext            = nullptr,
    .flags            = 0,
    .queueFamilyIndex = family_index,
    .queueCount       = 1,
    .pQueuePriorities = &priority,
  };

  std::array constexpr device_extensions{ "VK_KHR_swapchain" };
  VkDeviceCreateInfo const device_create_info{
    .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext                   = nullptr,
    .flags                   = 0,
    .queueCreateInfoCount    = 1,
    .pQueueCreateInfos       = &queue_create_info,
    .enabledExtensionCount   = std::size(device_extensions),
    .ppEnabledExtensionNames = std::data(device_extensions),
  };

  VkDevice device;
  vkCreateDevice(physical_device, &device_create_info, alloc, &device);

  return std::make_tuple(device, family_index);
}

auto vulkan::select_device_queue(
  u32 const family_index,
  VkDevice  logical_device
) -> VkQueue {
  VkQueue queue;
  vkGetDeviceQueue(logical_device, family_index, 0, &queue);
  return queue;
}

auto vulkan::create_command_pool(
  VkAllocationCallbacks *alloc,
  u32 const              family_index,
  VkDevice               logical_device
) -> VkCommandPool {

  VkCommandPoolCreateInfo const create_info{
    .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext            = nullptr,
    .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = family_index,
  };

  VkCommandPool pool;
  vkCreateCommandPool(logical_device, &create_info, alloc, &pool);
  return pool;
}


} // namespace gzn::gfx::backends::ctx
