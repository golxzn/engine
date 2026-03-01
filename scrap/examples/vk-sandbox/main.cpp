#include <gzn/app/view.hpp>
#include <gzn/fnd/owner.hpp>
#include <gzn/gfx/backends/ctx/vulkan.hpp>
#include <gzn/gfx/commands.hpp>
#include <gzn/gfx/context.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#define CHECK_VK_RESULT(_expr)                                        \
  if (result = _expr; result != VK_SUCCESS) {                         \
    std::fprintf(stderr, "Error executing %s: %i\n", #_expr, result); \
  }

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

static VkSurfaceKHR      vulkanSurface    = VK_NULL_HANDLE;
static VkPhysicalDevice  physDevice       = VK_NULL_HANDLE;
static VkDevice          device           = VK_NULL_HANDLE;
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

static void createSwapchain();
static void destroySwapchain();

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

    SwapchainElement *currentElement = &elements[currentFrame];

    CHECK_VK_RESULT(
      vkWaitForFences(device, 1, &currentElement->fence, 1, UINT64_MAX)
    );
    result = vkAcquireNextImageKHR(
      device,
      swapchain,
      UINT64_MAX,
      currentElement->startSemaphore,
      nullptr,
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

    SwapchainElement *element = &elements[imageIndex];

    if (element->lastFence) {
      CHECK_VK_RESULT(
        vkWaitForFences(device, 1, &element->lastFence, 1, UINT64_MAX)
      );
    }

    element->lastFence = currentElement->fence;

    CHECK_VK_RESULT(vkResetFences(device, 1, &currentElement->fence));

    VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    CHECK_VK_RESULT(vkBeginCommandBuffer(element->commandBuffer, &beginInfo));

    {
      VkClearValue const clearValue{
        .color{ 0.435f, 0.376f, 0.541f, 1.0f }
      };

      VkRenderPassBeginInfo const beginInfo{
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass  = renderPass,
        .framebuffer = element->framebuffer,
        .renderArea{
                    .offset{ .x = 0, .y = 0 },
                    .extent{ .width = width, .height = height },
                    },
        .clearValueCount = 1,
        .pClearValues    = &clearValue,
      };
      vkCmdBeginRenderPass(
        element->commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE
      );
      vkCmdEndRenderPass(element->commandBuffer);
    }

    CHECK_VK_RESULT(vkEndCommandBuffer(element->commandBuffer));

    VkPipelineStageFlags const waitStage{
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo const submitInfo{
      .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount   = 1,
      .pWaitSemaphores      = &currentElement->startSemaphore,
      .pWaitDstStageMask    = &waitStage,
      .commandBufferCount   = 1,
      .pCommandBuffers      = &element->commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores    = &currentElement->endSemaphore,
    };
    CHECK_VK_RESULT(
      vkQueueSubmit(queue, 1, &submitInfo, currentElement->fence)
    );

    VkPresentInfoKHR const presentInfo{
      .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores    = &currentElement->endSemaphore,
      .swapchainCount     = 1,
      .pSwapchains        = &swapchain,
      .pImageIndices      = &imageIndex,
    };
    result = vkQueuePresentKHR(queue, &presentInfo);

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

  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyDevice(device, nullptr);
}

static void createSwapchain() {
  VkResult result;

  {
    uint32_t formatCount;
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
      physDevice, vulkanSurface, &formatCount, nullptr
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

    format = chosenFormat.format;

    VkSurfaceCapabilitiesKHR capabilities;
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      physDevice, vulkanSurface, &capabilities
    ));

    imageCount = capabilities.minImageCount + 1 < capabilities.maxImageCount
                 ? capabilities.minImageCount + 1
                 : capabilities.minImageCount;

    VkSwapchainCreateInfoKHR const createInfo{
      .sType           = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface         = vulkanSurface,
      .minImageCount   = imageCount,
      .imageFormat     = chosenFormat.format,
      .imageColorSpace = chosenFormat.colorSpace,
      .imageExtent{ .width = width, .height = height },
      .imageArrayLayers = 1,
      .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform     = capabilities.currentTransform,
      .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode      = VK_PRESENT_MODE_MAILBOX_KHR,
      .clipped          = 1,
    };

    CHECK_VK_RESULT(
      vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain)
    );
  }

  {
    VkAttachmentDescription const attachment{
      .flags          = 0,
      .format         = format,
      .samples        = VK_SAMPLE_COUNT_1_BIT,
      .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference const attachmentRef{
      .attachment = 0,
      .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription const subpass{
      .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments    = &attachmentRef,
    };
    VkRenderPassCreateInfo const createInfo{
      .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .flags           = 0,
      .attachmentCount = 1,
      .pAttachments    = &attachment,
      .subpassCount    = 1,
      .pSubpasses      = &subpass,
    };
    CHECK_VK_RESULT(
      vkCreateRenderPass(device, &createInfo, nullptr, &renderPass)
    );
  }

  CHECK_VK_RESULT(
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr)
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
      VkCommandBufferAllocateInfo const allocInfo{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
      };

      vkAllocateCommandBuffers(device, &allocInfo, &elements[i].commandBuffer);
    }

    elements[i].image = images[i];

    {
      VkImageViewCreateInfo const createInfo{
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = elements[i].image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = format,
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

      CHECK_VK_RESULT(
        vkCreateImageView(device, &createInfo, nullptr, &elements[i].imageView)
      );
    }

    {
      VkFramebufferCreateInfo const createInfo{
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = renderPass,
        .attachmentCount = 1,
        .pAttachments    = &elements[i].imageView,
        .width           = width,
        .height          = height,
        .layers          = 1,
      };

      CHECK_VK_RESULT(vkCreateFramebuffer(
        device, &createInfo, nullptr, &elements[i].framebuffer
      ));
    }

    {
      VkSemaphoreCreateInfo const createInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      };

      CHECK_VK_RESULT(vkCreateSemaphore(
        device, &createInfo, nullptr, &elements[i].startSemaphore
      ));
    }

    {
      VkSemaphoreCreateInfo const createInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      };

      CHECK_VK_RESULT(vkCreateSemaphore(
        device, &createInfo, nullptr, &elements[i].endSemaphore
      ));
    }

    {
      VkFenceCreateInfo const createInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
      };

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
