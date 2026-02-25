#include <gzn/app/view.hpp>
#include <gzn/fnd/owner.hpp>
#include <gzn/gfx/backends/ctx/vulkan.hpp>
#include <gzn/gfx/commands.hpp>
#include <gzn/gfx/context.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#define CHECK_VK_RESULT(_expr)                          \
  result = _expr;                                       \
  if (result != VK_SUCCESS) {                           \
    printf("Error executing %s: %i\n", #_expr, result); \
  }

#define GET_EXTENSION_FUNCTION(_id)                    \
  ((PFN_##_id)(vkGetInstanceProcAddr(instance, #_id)))

struct SwapchainElement {
  VkCommandBuffer commandBuffer;
  VkImage         image;
  VkImageView     imageView;
  VkFramebuffer   framebuffer;
  VkSemaphore     startSemaphore;
  VkSemaphore     endSemaphore;
  VkFence         fence;
  VkFence         lastFence;
};

std::array constexpr deviceExtensionNames{ "VK_KHR_swapchain" };
static VkSurfaceKHR      vulkanSurface    = VK_NULL_HANDLE;
static VkPhysicalDevice  physDevice       = VK_NULL_HANDLE;
static VkDevice          device           = VK_NULL_HANDLE;
static uint32_t          queueFamilyIndex = 0;
static VkQueue           queue            = VK_NULL_HANDLE;
static VkCommandPool     commandPool      = VK_NULL_HANDLE;
static VkSwapchainKHR    swapchain        = VK_NULL_HANDLE;
static VkRenderPass      renderPass       = VK_NULL_HANDLE;
static SwapchainElement *elements         = NULL;
static int               quit             = 0;
static int               readyToResize    = 0;
static int               resize           = 0;
static int               newWidth         = 0;
static int               newHeight        = 0;
static VkFormat          format           = VK_FORMAT_UNDEFINED;
static uint32_t          width            = 1600;
static uint32_t          height           = 900;
static uint32_t          currentFrame     = 0;
static uint32_t          imageIndex       = 0;
static uint32_t          imageCount       = 0;

static void createSwapchain() {
  VkResult result;

  {
    VkSurfaceCapabilitiesKHR capabilities;
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      physDevice, vulkanSurface, &capabilities
    ));

    uint32_t formatCount;
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
      physDevice, vulkanSurface, &formatCount, NULL
    ));

    VkSurfaceFormatKHR formats[formatCount];
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
      physDevice, vulkanSurface, &formatCount, formats
    ));

    VkSurfaceFormatKHR chosenFormat = formats[0];

    for (uint32_t i = 0; i < formatCount; i++) {
      if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
        chosenFormat = formats[i];
        break;
      }
    }

    format     = chosenFormat.format;

    imageCount = capabilities.minImageCount + 1 < capabilities.maxImageCount
                 ? capabilities.minImageCount + 1
                 : capabilities.minImageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType             = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface           = vulkanSurface;
    createInfo.minImageCount     = imageCount;
    createInfo.imageFormat       = chosenFormat.format;
    createInfo.imageColorSpace   = chosenFormat.colorSpace;
    createInfo.imageExtent.width = width;
    createInfo.imageExtent.height = height;
    createInfo.imageArrayLayers   = 1;
    createInfo.imageUsage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform       = capabilities.currentTransform;
    createInfo.compositeAlpha     = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode        = VK_PRESENT_MODE_MAILBOX_KHR;
    createInfo.clipped            = 1;

    CHECK_VK_RESULT(
      vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain)
    );
  }

  {
    VkAttachmentDescription attachment{};
    attachment.format         = format;
    attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attachmentRef{};
    attachmentRef.attachment = 0;
    attachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &attachmentRef;

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.flags           = 0;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments    = &attachment;
    createInfo.subpassCount    = 1;
    createInfo.pSubpasses      = &subpass;

    CHECK_VK_RESULT(
      vkCreateRenderPass(device, &createInfo, NULL, &renderPass)
    );
  }

  CHECK_VK_RESULT(
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL)
  );

  VkImage images[imageCount];
  CHECK_VK_RESULT(
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images)
  );

  elements = (SwapchainElement *)malloc(
    imageCount * sizeof(struct SwapchainElement)
  );

  for (uint32_t i = 0; i < imageCount; i++) {
    {
      VkCommandBufferAllocateInfo allocInfo{};
      allocInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.commandPool = commandPool;
      allocInfo.commandBufferCount = 1;
      allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

      vkAllocateCommandBuffers(device, &allocInfo, &elements[i].commandBuffer);
    }

    elements[i].image = images[i];

    {
      VkImageViewCreateInfo createInfo{};
      createInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      createInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;
      createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.subresourceRange.baseMipLevel   = 0;
      createInfo.subresourceRange.levelCount     = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount     = 1;
      createInfo.image                           = elements[i].image;
      createInfo.format                          = format;
      createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;

      CHECK_VK_RESULT(
        vkCreateImageView(device, &createInfo, NULL, &elements[i].imageView)
      );
    }

    {
      VkFramebufferCreateInfo createInfo{};
      createInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      createInfo.renderPass      = renderPass;
      createInfo.attachmentCount = 1;
      createInfo.pAttachments    = &elements[i].imageView;
      createInfo.width           = width;
      createInfo.height          = height;
      createInfo.layers          = 1;

      CHECK_VK_RESULT(vkCreateFramebuffer(
        device, &createInfo, NULL, &elements[i].framebuffer
      ));
    }

    {
      VkSemaphoreCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      CHECK_VK_RESULT(vkCreateSemaphore(
        device, &createInfo, NULL, &elements[i].startSemaphore
      ));
    }

    {
      VkSemaphoreCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      CHECK_VK_RESULT(
        vkCreateSemaphore(device, &createInfo, NULL, &elements[i].endSemaphore)
      );
    }

    {
      VkFenceCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

      CHECK_VK_RESULT(
        vkCreateFence(device, &createInfo, NULL, &elements[i].fence)
      );
    }

    elements[i].lastFence = VK_NULL_HANDLE;
  }
}

static void destroySwapchain() {
  for (uint32_t i = 0; i < imageCount; i++) {
    vkDestroyFence(device, elements[i].fence, NULL);
    vkDestroySemaphore(device, elements[i].endSemaphore, NULL);
    vkDestroySemaphore(device, elements[i].startSemaphore, NULL);
    vkDestroyFramebuffer(device, elements[i].framebuffer, NULL);
    vkDestroyImageView(device, elements[i].imageView, NULL);
    vkFreeCommandBuffers(device, commandPool, 1, &elements[i].commandBuffer);
  }

  free(elements);

  vkDestroyRenderPass(device, renderPass, NULL);

  vkDestroySwapchainKHR(device, swapchain, NULL);
}

int main() {
  using namespace gzn;

  fnd::stack_arena_allocator<512> view_alloc{};

  app::view_info const view_info{
    .title = "vk test",
    .size{ width, height },
  };
  auto view{ app::view::make<fnd::stack_owner>(view_alloc, view_info) };
  if (!view.is_alive()) {
    std::fprintf(stderr, "Failed to make app::view!\n");
    return EXIT_FAILURE;
  }

  fnd::base_allocator gfx_alloc{};
  auto constexpr backend_type{ gfx::backend_type::vulkan };
  gfx::context_info ctx_info{
    .backend = backend_type,
    .app_name{ "wayland-vulkan-testing" },
    .surface_builder{ view->get_surface_builder(gfx_alloc, backend_type) },
    .extensions{ view->get_required_extensions() }
  };
  auto ctx{ gfx::context::make(gfx_alloc, std::move(ctx_info)) };
  if (!ctx.is_valid()) {
    std::fprintf(stderr, "Failed to make gfx::context!\n");
    return EXIT_FAILURE;
  }

  auto vk{ ctx.data().as<gfx::backends::ctx::vulkan>() };
  vulkanSurface = (VkSurfaceKHR)ctx.get_surface_proxy()->get_handle();
  physDevice    = vk->physical_device;
  device        = vk->logical_device;
  queue         = vk->queue;
  commandPool   = vk->command_pool;

  VkResult result;

  createSwapchain();

  while (!quit) {
    if (readyToResize && resize) {
      width  = newWidth;
      height = newHeight;

      CHECK_VK_RESULT(vkDeviceWaitIdle(device));

      destroySwapchain();
      createSwapchain();

      currentFrame  = 0;
      imageIndex    = 0;

      readyToResize = 0;
      resize        = 0;

      // wl_surface_commit(surface);
    }

    struct SwapchainElement *currentElement = &elements[currentFrame];

    CHECK_VK_RESULT(
      vkWaitForFences(device, 1, &currentElement->fence, 1, UINT64_MAX)
    );
    result = vkAcquireNextImageKHR(
      device,
      swapchain,
      UINT64_MAX,
      currentElement->startSemaphore,
      NULL,
      &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
      CHECK_VK_RESULT(vkDeviceWaitIdle(device));
      destroySwapchain();
      createSwapchain();
      continue;
    } else if (result < 0) {
      CHECK_VK_RESULT(result);
    }

    struct SwapchainElement *element = &elements[imageIndex];

    if (element->lastFence) {
      CHECK_VK_RESULT(
        vkWaitForFences(device, 1, &element->lastFence, 1, UINT64_MAX)
      );
    }

    element->lastFence = currentElement->fence;

    CHECK_VK_RESULT(vkResetFences(device, 1, &currentElement->fence));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    CHECK_VK_RESULT(vkBeginCommandBuffer(element->commandBuffer, &beginInfo));

    {
      VkClearValue clearValue = {
        { 0.435f, 0.376f, 0.541f, 1.0f }
      };

      VkRenderPassBeginInfo beginInfo{};
      beginInfo.sType               = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      beginInfo.renderPass          = renderPass;
      beginInfo.framebuffer         = element->framebuffer;
      beginInfo.renderArea.offset.x = 0;
      beginInfo.renderArea.offset.y = 0;
      beginInfo.renderArea.extent.width  = width;
      beginInfo.renderArea.extent.height = height;
      beginInfo.clearValueCount          = 1;
      beginInfo.pClearValues             = &clearValue;

      vkCmdBeginRenderPass(
        element->commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE
      );
      vkCmdEndRenderPass(element->commandBuffer);
    }

    CHECK_VK_RESULT(vkEndCommandBuffer(element->commandBuffer));

    VkPipelineStageFlags const
      waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &currentElement->startSemaphore;
    submitInfo.pWaitDstStageMask    = &waitStage;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &element->commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &currentElement->endSemaphore;

    CHECK_VK_RESULT(
      vkQueueSubmit(queue, 1, &submitInfo, currentElement->fence)
    );

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &currentElement->endSemaphore;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapchain;
    presentInfo.pImageIndices      = &imageIndex;

    result                         = vkQueuePresentKHR(queue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
      CHECK_VK_RESULT(vkDeviceWaitIdle(device));
      destroySwapchain();
      createSwapchain();
    } else if (result < 0) {
      CHECK_VK_RESULT(result);
    }

    currentFrame = (currentFrame + 1) % imageCount;

    gfx::cmd::present(ctx);
  }

  CHECK_VK_RESULT(vkDeviceWaitIdle(device));

  destroySwapchain();

  vkDestroyCommandPool(device, commandPool, NULL);
  vkDestroyDevice(device, NULL);

  return 0;
}

/*

#include <unordered_set>

#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>
#include <gzn/application>
#include <gzn/foundation>
#include <gzn/graphics>
#include <vulkan/vulkan.hpp>

namespace gzn {

namespace constants {

constexpr std::array validation_layers{ "VK_LAYER_KHRONOS_validation" };

} // namespace constants

class vulkan_instance final {
public:
  vulkan_instance();
  ~vulkan_instance();

  vulkan_instance(vulkan_instance const &)                = delete;
  vulkan_instance &operator=(vulkan_instance const &)     = delete;

  vulkan_instance(vulkan_instance &&) noexcept            = delete;
  vulkan_instance &operator=(vulkan_instance &&) noexcept = delete;

  [[nodiscard]] static auto handle() noexcept { return m_instance; }

private:
  inline static VkInstance m_instance{ VK_NULL_HANDLE };

#if defined(GZN_DEBUG)
  VkDebugUtilsMessengerEXT m_debug_messenger{ VK_NULL_HANDLE };

  VkDebugUtilsMessengerCreateInfoEXT make_debug_messenger_create_info();
  void                               construct_debug_messenger();
  bool                               check_validation_layer_support() const;
#endif // defined(GZN_DEBUG)

  void construct_instance();

  auto required_extensions() const -> std::vector<char const *>;
  void has_glfw_required_instance_extensions() const;
};

class vulkan_instance_error : public std::runtime_error {
public:
  using base_type = std::runtime_error;
  using base_type::runtime_error;
};

//////////////////////////////// IMPL //////////////////////////////////

#pragma region utilities

namespace {

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
  VkDebugUtilsMessageTypeFlagsEXT             type,
  VkDebugUtilsMessengerCallbackDataEXT const *callback_data,
  void *
) {
  static constexpr auto to_str{ [](auto const severity) {
    switch (severity) {
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT   : return "INFO";
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "WARN";
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT  : return "ERR ";
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "VERB";
      default                                             : break;
    }
    return "UNKN";
  } };

  std::fprintf(
    stderr, "[VULKAN][%s] %s\n", to_str(severity), callback_data->pMessage
  );
  return VK_FALSE;
}

#if defined(GZN_DEBUG)

VkDebugUtilsMessengerEXT create_debug_utils_messenger(
  VkInstance                                instance,
  const VkDebugUtilsMessengerCreateInfoEXT *create_info,
  const VkAllocationCallbacks              *allocator
) {
  auto const constructor{ reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
    vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
  ) };

  if (constructor == nullptr) { return nullptr; }

  if (VkDebugUtilsMessengerEXT messenger;
      VK_SUCCESS ==
      constructor(instance, create_info, allocator, &messenger)) {
    return messenger;
  }
  return nullptr;
}

void destroy_debug_utils_messenger(
  VkInstance                   instance,
  VkDebugUtilsMessengerEXT     messenger,
  VkAllocationCallbacks const *allocator
) {
  auto const destructor{ reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
    vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")
  ) };

  if (destructor != nullptr) { destructor(instance, messenger, allocator); }
}


#endif // defined(GZN_DEBUG)

} // anonymous namespace

#pragma endregion utilities

vulkan_instance::vulkan_instance() {
  if (m_instance != VK_NULL_HANDLE) {
    throw vulkan_instance_error{ "Vulkan Instance is already exist!" };
  }

  construct_instance();
#if defined(GZN_DEBUG)
  construct_debug_messenger();
#endif // defined(GZN_DEBUG)
}

vulkan_instance::~vulkan_instance() {
#if defined(GZN_DEBUG)
  destroy_debug_utils_messenger(m_instance, m_debug_messenger, nullptr);
#endif // defined(GZN_DEBUG)

  vkDestroyInstance(m_instance, nullptr);
}

#if defined(GZN_DEBUG)

VkDebugUtilsMessengerCreateInfoEXT vulkan_instance::
  make_debug_messenger_create_info() {
  return VkDebugUtilsMessengerCreateInfoEXT{
    .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debug_callback,
    .pUserData       = nullptr
  };
}

void vulkan_instance::construct_debug_messenger() {
  auto const create_info{ make_debug_messenger_create_info() };
  m_debug_messenger = create_debug_utils_messenger(
    m_instance, &create_info, nullptr
  );
  if (m_debug_messenger == nullptr) {
    throw vulkan_instance_error{ "Failed to construct the debug messenger." };
  }
}

bool vulkan_instance::check_validation_layer_support() const {
  u32 layers_count{};
  vkEnumerateInstanceLayerProperties(&layers_count, nullptr);

  std::vector<VkLayerProperties> properties(layers_count);
  vkEnumerateInstanceLayerProperties(&layers_count, std::data(properties));

  for (std::string_view const layer_name : constants::validation_layers) {
    bool found{ false };
    for (auto const &property : properties) {
      if (layer_name == property.layerName) {
        found = true;
        break;
      }
    }

    if (!found) { return false; }
  }

  return true;
}

#endif // defined(GZN_DEBUG)

void vulkan_instance::construct_instance() {
#if defined(GZN_DEBUG)
  if (!check_validation_layer_support()) {
    throw vulkan_instance_error{
      "Validation layers are not available, but required."
    };
  }
  auto const debug_create_info{ make_debug_messenger_create_info() };
#endif // defined(GZN_DEBUG)

  const VkApplicationInfo application_info{
    .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName   = "testing",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName        = "vulkan-course-engine",
    .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion         = VK_API_VERSION_1_0
  };

  auto const                 extensions{ required_extensions() };
  VkInstanceCreateInfo const create_info{
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#if defined(GZN_DEBUG)
    .pNext             = &debug_create_info,
    .pApplicationInfo  = &application_info,
    .enabledLayerCount = static_cast<u32>(
      std::size(constants::validation_layers)
    ),
    .ppEnabledLayerNames = std::data(constants::validation_layers),
#else
    .pApplicationInfo = &application_info,
#endif // defined(GZN_DEBUG)
    .enabledExtensionCount   = static_cast<u32>(std::size(extensions)),
    .ppEnabledExtensionNames = std::data(extensions)
  };

  if (VK_SUCCESS != vkCreateInstance(&create_info, nullptr, &m_instance)) {
    throw vulkan_instance_error{ "Failed to create the vulkan instance." };
  }

  has_glfw_required_instance_extensions();
}

auto vulkan_instance::required_extensions() const
  -> std::vector<char const *> {
  u32         glfw_extensions_count{};
  char const *glfw_extensions[]{ VK_KHR_SURFACE_EXTENSION_NAME,
                                 VK_KHR_XLIB_SURFACE_EXTENSION_NAME };

#if defined(GZN_DEBUG)
  std::vector<const char *> extensions(glfw_extensions_count + 1);
  std::copy_n(glfw_extensions, glfw_extensions_count, std::data(extensions));
  extensions.back() = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
  return extensions;
#else
  return std::vector<const char *>(
    glfw_extensions, glfw_extensions + glfw_extensions_count
  );
#endif // defined(GZN_DEBUG)
}

void vulkan_instance::has_glfw_required_instance_extensions() const {
  u32 extensions_count{};
  vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);

  std::vector<VkExtensionProperties> extensions(extensions_count);
  vkEnumerateInstanceExtensionProperties(
    nullptr, &extensions_count, std::data(extensions)
  );

  std::unordered_set<std::string_view> const available{
    [](auto const &extensions) {
      std::unordered_set<std::string_view> result;
      for (auto const &extension : extensions) {
        result.emplace(extension.extensionName);
      }
      return result;
    }(extensions)
  };
  auto const are_available{ [&available](auto const &extension) {
    return available.contains(extension);
  } };

  auto const required{ required_extensions() };
  if (std::all_of(std::begin(required), std::end(required), are_available)) {
    return;
  }


  std::string missing{};
  missing.reserve(200); // 200 to prevent a lot of reallocations
  for (auto const &extension : required) {
    if (!available.contains(extension)) {
      missing += extension;
      missing += ", ";
    }
  }
  if (!std::empty(missing)) {
    missing.resize(std::size(missing) - std::size(", "));
    throw vulkan_instance_error{
      fmt::format("Missing required extensions: {}", std::move(missing))
    };
  }
}


} // namespace gzn

int main() {
  using namespace gzn;
  fnd::base_allocator alloc{};

  auto view{
    app::view::make<fnd::stack_owner>(alloc, { .size{ 1920, 1080 } }
     )
  };
  if (!view.is_alive()) { return EXIT_FAILURE; }

  auto constexpr backend{ gfx::backend_type::vulkan };
  auto render{ gfx::context::make<fnd::stack_owner>(
    alloc,
    gfx::context_info{
      .backend = backend,
      .surface_builder{ view->get_surface_proxy_builder(alloc, backend) },
    }
  ) };
  if (!render.is_alive()) { return EXIT_FAILURE; }

#pragma region INSTANCE

#if defined(GZN_DEBUG)

#endif // defined(GZN_DEBUG)

#pragma endregion INSTANCE

  // clang-format off
  struct vertex {
    glm::vec3 pos{};
    glm::vec2 uv{};
  };
  fnd::raw_data static const vertices{ {
    vertex{ .pos{ -0.5f, -0.5f, 0.0f }, .uv{ 0.0f, 0.0f } },
    vertex{ .pos{  0.5f,  0.5f, 0.0f }, .uv{ 1.0f, 1.0f } },
    vertex{ .pos{ -0.5f,  0.5f, 0.0f }, .uv{ 0.0f, 1.0f } },
    vertex{ .pos{  0.5f, -0.5f, 0.0f }, .uv{ 1.0f, 0.0f } }
  } };

  fnd::raw_data static const indices{ c_array<u16, 6>{ 0, 1, 2, 0, 3, 1 } };
  // clang-format on


  bool       running{ true };
  app::event event{};
  while (running) {
    while (view->take_next_event(event)) {
      if (auto key{ std::get_if<app::event_key_pressed>(&event) }; key) {
        if (fnd::util::any_from(key->key, app::keys::q, app::keys::escape)) {
          running = false;
          break;
        }
      }
    }


    gfx::cmd::present(*render);
  }
}

*/
