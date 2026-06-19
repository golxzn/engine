#include "gzn/gfx/backends/ctx/vulkan.hpp"

#if defined(GZN_GFX_BACKEND_VULKAN)

#  include <algorithm>

#  include <glm/vec3.hpp>
#  include <vulkan/vulkan_wayland.h>

#  include "gzn/fnd/containers/dynamic-array.hpp"
#  include "gzn/fnd/util/unsafe-any-ref.hpp"
#  include "gzn/gfx/context.hpp"
#  include "gzn/gfx/gpu-info.hpp"

namespace gzn::gfx::backends::ctx {

#  define GET_EXTENSION_FUNCTION(_id)                    \
    ((PFN_##_id)(vkGetInstanceProcAddr(instance, #_id)))

namespace {

inline constexpr cstr THIS_MODULE{ "gfx::ctx::vulkan" };

int glad_vk_version{};

vulkan g_ctx{};

// VkAllocationCallbacks g_default_callbacks{
//   .pUserData             = nullptr,
//   .pfnAllocation         = nullptr,
//   .pfnReallocation       = nullptr,
//   .pfnFree               = nullptr,
//   .pfnInternalAllocation = nullptr,
//   .pfnInternalFree       = nullptr,
// };

constexpr auto convert_format(format_type format) -> VkFormat {
  switch (format) {
    using enum format_type;
    case rgb_u8  : return VK_FORMAT_R8G8B8_UINT;
    case rgb_s8  : return VK_FORMAT_R8G8B8_SINT;
    case rgba_u8 : return VK_FORMAT_R8G8B8A8_UINT;
    case rgba_s8 : return VK_FORMAT_R8G8B8A8_SINT;

    case rgb_f32 : return VK_FORMAT_R32G32B32_SFLOAT;
    case rgba_f32: return VK_FORMAT_R32G32B32A32_SFLOAT;

    default      : break;
  }
  return VK_FORMAT_UNDEFINED;
}

constexpr auto convert_extent(glm::u32vec2 size) noexcept -> VkExtent2D {
  return VkExtent2D{ size.x, size.y };
}

constexpr auto convert_extent(glm::u32vec3 size) noexcept -> VkExtent3D {
  return VkExtent3D{ size.x, size.y, size.z };
}

struct surface_builder {
  auto operator()(api::wayland &back) -> VkResult {
    VkWaylandSurfaceCreateInfoKHR const create_info{
      .sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
      .pNext   = nullptr,
      .flags   = 0,
      .display = static_cast<wl_display *>(back.wl_display),
      .surface = static_cast<wl_surface *>(back.wl_surface),
    };
    return vkCreateWaylandSurfaceKHR(
      g_ctx.instance, &create_info, g_ctx.allocator, &g_ctx.surface
    );
  }

  auto operator()(auto &) -> VkResult {
    gzn_do_assertion("UNIMPLEMENTED API!");
    return VK_INCOMPLETE;
  }
};

} // namespace

struct struct_sizes {
  static constexpr auto pipeline_bytes_count{ sizeof(vk_pipeline) };
  static constexpr auto buffer_bytes_count{ sizeof(vk_buffer) };
};

struct gen_counter_size {
  static constexpr auto pipeline_bytes_count{ sizeof(u16) };
  static constexpr auto buffer_bytes_count{ sizeof(u16) };
};

struct offset_accumulator {
  fnd::span<byte> iter;

  template<class T>
  constexpr auto set(usize const count) noexcept {
    auto place{ iter.subrange(0, count) };
    iter = iter.subrange(count);
    return place;
  }
};

void vulkan::load() {
  glad_vk_version = gladLoaderLoadVulkan(
    VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE
  );
}

void vulkan::unload() {
  gladLoaderUnloadVulkan();
  glad_vk_version = 0;
}

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
    info.log_func.err(THIS_MODULE, "Vulkan backend is already exists!");
    /// @todo think: nullptr or existing instance?
    return nullptr;
  }
  // int const version{
  //   gladLoaderLoadVulkan(VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE)
  // };
  if (!glad_vk_version) {
    info.log_func.fatal(THIS_MODULE, "Failed to initialize GLAD");
    return nullptr;
  }

  VkAllocationCallbacks *alloc{ nullptr /*&g_default_callbacks*/ };
  if (extra_storage != nullptr) {
    auto const extra{ extra_storage.as<vulkan_extra_data>() };
    alloc = extra->allocator;
  }

  auto instance{ make_instance(alloc, info) };
  if (instance == VK_NULL_HANDLE) { return nullptr; }

#  if defined(GZN_DEBUG)
  auto debug_messenger{ make_debug_messenger(alloc, instance) };
#  endif // defined(GZN_DEBUG)

  g_ctx = vulkan{
    .allocator = alloc,
    .instance  = instance,
#  if defined(GZN_DEBUG)
    .debug_messenger = debug_messenger,
#  endif // defined(GZN_DEBUG),
    .log{ info.log_func },
  }; // NOLINT
  return &g_ctx;
}

auto vulkan::setup(
  fnd::span<byte>     storage,
  context_info const &info,
  surface_proxy      &surface_wrapper
) -> bool {
  if (g_ctx.instance == VK_NULL_HANDLE) { return false; }

  auto const &caps{ info.capacities };
  auto const  required_space{ calc_required_space_for(caps) };
  if (std::size(storage) != required_space) {
    g_ctx.log.err(
      THIS_MODULE,
      "The storage size (%zu) is not enough for required caps (%zu)",
      std::size(storage),
      required_space
    );
    return false;
  }

  auto alloc{ g_ctx.allocator };

  auto physical_device{ select_physical_device(g_ctx.instance, info) };
  if (physical_device == VK_NULL_HANDLE) {
    g_ctx.log.err(THIS_MODULE, "Failed to select physical device");
    return false;
  }

  auto handle{ surface_wrapper.get_handle() };
  // handle.visit
  if (VK_SUCCESS != std::visit(surface_builder{}, handle)) {
    g_ctx.log.err(
      THIS_MODULE,
      "Failed to construct surface! Check the handle in surface_proxy."
    );
    return false;
  }

  auto [logical_device, queue_index]{
    select_logical_device(alloc, physical_device, g_ctx.surface)
  };

  gladLoaderLoadVulkan(g_ctx.instance, physical_device, logical_device);

  auto queue{ select_device_queue(queue_index, logical_device) };
  auto command_pool{ create_command_pool(alloc, queue_index, logical_device) };

  using pool_size_t = fnd::handle_index_type;
  offset_accumulator off{ .iter{ storage } };
  auto const offset_pipelines{ off.set<vk_pipeline>(caps.pipelines_count) };
  auto const offset_buffers{ off.set<vk_buffer>(caps.buffers_count) };
  auto const offset_samplers{ off.set<vk_sampler>(caps.samples_count) };

  g_ctx = vulkan{
    .allocator = g_ctx.allocator,
    .instance  = g_ctx.instance,
#  if defined(GZN_DEBUG)
    .debug_messenger = g_ctx.debug_messenger,
#  endif  // defined(GZN_DEBUG)
    .log{ g_ctx.log },

    .surface         = g_ctx.surface,
    .physical_device = physical_device,
    .logical_device  = logical_device,
    .queue           = queue,
    .command_pool    = command_pool,

    .storage{ storage },
    .pipelines{ offset_pipelines, caps.pipelines_count.value<pool_size_t>() },
    .buffers{ offset_buffers, caps.buffers_count.value<pool_size_t>() },
    .samplers{ offset_samplers, caps.samples_count.value<pool_size_t>() },
  };
  return true;
}

void vulkan::destroy() {
  vkDestroyDevice(g_ctx.logical_device, g_ctx.allocator);

  vkDestroySurfaceKHR(g_ctx.instance, g_ctx.surface, g_ctx.allocator);

#  if defined(GZN_DEBUG)
  {
    auto instance{ g_ctx.instance };
    GET_EXTENSION_FUNCTION(vkDestroyDebugUtilsMessengerEXT)(
      instance, g_ctx.debug_messenger, g_ctx.allocator
    );
  }
#  endif // defined(GZN_DEBUG)

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


  static fnd::in_stack_allocator<sizeof(VkLayerProperties) * 25> tmp{};
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

#  if defined(GZN_DEBUG)

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
    .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
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

#  endif // defined(GZN_DEBUG)

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
    fnd::span<gpu_info const>{
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

auto vulkan::create_swapchain(
  VkAllocationCallbacks *alloc,
  fnd::log_func         &log,
  swapchain_info const  &info,
  VkSurfaceKHR           surface,
  VkPhysicalDevice       physical_device,
  VkDevice               device
) -> vk_swapchain {
  /*
  static constexpr auto convert_present_mode{ [](present_mode mode) {
    switch (mode) {
      using enum present_mode;
      case immediate: return VK_PRESENT_MODE_IMMEDIATE_KHR;
      case mailbox  : return VK_PRESENT_MODE_MAILBOX_KHR;
      case fifo     : return VK_PRESENT_MODE_FIFO_KHR;
    }
    return VK_PRESENT_MODE_MAILBOX_KHR;
  } };

  u32 format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(
    physical_device, surface, &format_count, nullptr
  );

  static fnd::stack_arena_allocator<sizeof(VkSurfaceFormatKHR) * 25> tmp{};
  fnd::dynamic_array<VkSurfaceFormatKHR, decltype(tmp)>              formats{
    tmp, static_cast<usize>(format_count)
  };

  vkGetPhysicalDeviceSurfaceFormatsKHR(
    physical_device, surface, &format_count, std::data(formats)
  );

  auto constexpr no_format_idx{ std::numeric_limits<u32>::max() };
  auto const asked_format{ convert_format(info.image_format) };

  u32 chosen_format_idx{ no_format_idx };
  for (u32 i{}; i < format_count; ++i) {
    if (formats[i].format == asked_format) {
      chosen_format_idx = i;
      break;
    }
  }
  if (chosen_format_idx == no_format_idx) {
    chosen_format_idx = 0;
    log.warn(
      THIS_MODULE,
      "Requested image format is not supported! "
      "0x%Xu native format will be used",
      static_cast<u32>(formats[chosen_format_idx].format)
    );
  }

  auto const &surface_format{ formats[chosen_format_idx] };

  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    physical_device, surface, &capabilities
  );

  auto const asked_image_count{ static_cast<u32>(info.image_count) };
  auto const img_count{ std::clamp(
    asked_image_count, capabilities.minImageCount, capabilities.maxImageCount
  ) };
  if (img_count != asked_image_count) {
    log.warn(
      THIS_MODULE,
      "GPU does not support %u images count. %u will be used",
      asked_image_count,
      img_count
    );
  }

  VkSwapchainCreateInfoKHR const create_info{
    .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext            = nullptr,
    .flags            = 0,
    .surface          = surface,
    .minImageCount    = img_count,
    .imageFormat      = surface_format.format,
    .imageColorSpace  = surface_format.colorSpace,
    .imageExtent      = convert_extent(info.resolution),
    .imageArrayLayers = 1,
    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .preTransform     = capabilities.currentTransform,
    .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode      = convert_present_mode(info.present),
    .clipped          = static_cast<VkBool32>(info.clipped),
  };

  VkSwapchainKHR swapchain;
  vkCreateSwapchainKHR(device, &create_info, alloc, &swapchain);

  u32 actual_img_count;
  vkGetSwapchainImagesKHR(device, swapchain, &actual_img_count, nullptr);
  actual_img_count = std::min(actual_img_count, max_swapchain_elements);

  std::array<VkImage, max_swapchain_elements> images;
  vkGetSwapchainImagesKHR(
    device, swapchain, &actual_img_count, std::data(images)
  );


  swapchain_elements_array elements{};
  for (u32 i{}; i < actual_img_count; ++i) {
    auto &element{ elements[i] };
    element.image = images[i];

    {
      VkCommandBufferAllocateInfo const alloc_info{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
      };

      vkAllocateCommandBuffers(device, &alloc_info, &element.cmd_buffer);
    }

    {
      VkImageViewCreateInfo const createInfo{
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0,
        .image    = elements[i].image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = create_info.imageFormat,
        .components{
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
        .subresourceRange{
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                    },
      };

      vkCreateImageView(device, &createInfo, alloc, &element.image_view);
    }

    {
      VkFramebufferCreateInfo const createInfo{
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = renderPass,
        .attachmentCount = 1,
        .pAttachments    = &element.image_view,
        .width           = create_info.imageExtent.width,
        .height          = create_info.imageExtent.height,
        .layers          = 1,
      };

      vkCreateFramebuffer(device, &createInfo, alloc, &element.framebuffer);
    }

    {
      VkSemaphoreCreateInfo const createInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
      };

      vkCreateSemaphore(device, &createInfo, alloc, &element.start_semaphore);
      vkCreateSemaphore(device, &createInfo, alloc, &element.end_semaphore);
    }
    {
      VkFenceCreateInfo const createInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
      };

      vkCreateFence(device, &createInfo, alloc, &element.fence);
    }
  }

  return vk_swapchain{
    .handle       = swapchain,
    .images_count = actual_img_count,
    .format       = create_info.imageFormat,
    .extent       = create_info.imageExtent,
  };
  */
}


} // namespace gzn::gfx::backends::ctx

#endif // defined(GZN_GFX_BACKEND_VULKAN)
