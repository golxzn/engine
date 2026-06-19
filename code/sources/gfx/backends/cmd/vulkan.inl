#if defined(GZN_GFX_BACKEND_VULKAN)

#  include "gzn/gfx/backends/ctx/vulkan.hpp"
#  include "gzn/gfx/commands.hpp"
#  include "gzn/gfx/context.hpp"

namespace gzn::gfx::backends {

struct vulkan {
  static void begin_frame(context &ctx);
  static void end_frame(context &ctx);

  static void begin_pass(context &ctx, cmd_begin_render_pass const &info);
  static void end_pass(context &ctx, cmd_end_render_pass const &info);
};

gzn_inline void vulkan::begin_frame(context &ctx) {
  gzn_assertion(
    !ctx.is_valid(), R"("begin_frame" was called on invalid context!)"
  );

  auto data{ ctx.data().as<ctx::vulkan>() };

  VkCommandBufferInheritanceInfo const inheritance_info{
    .sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
    .pNext                = nullptr,
    .renderPass           = VK_NULL_HANDLE,
    .subpass              = 0u,
    .framebuffer          = VK_NULL_HANDLE,
    .occlusionQueryEnable = VK_FALSE,
    .queryFlags           = 0u,
    .pipelineStatistics   = 0u,

  };

  VkCommandBufferBeginInfo const begin_info{
    .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext            = nullptr,
    .flags            = 0,
    .pInheritanceInfo = &inheritance_info,
  };

  // get swapchain
  // VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo * pBeginInfo
  // vkBeginCommandBuffer(data->command_buffer, &begin_info);
}

gzn_inline void vulkan::end_frame(context &ctx) {
  gzn_assertion(
    !ctx.is_valid(), R"("end_frame" was called on invalid context!)"
  );

  auto data{ ctx.data().as<ctx::vulkan>() };

  // Somehow get current cmd buffer
  // vkEndCommandBuffer(command_buffer);
}

gzn_inline void vulkan::begin_pass(
  context                     &ctx,
  cmd_begin_render_pass const &info
) {}

gzn_inline void vulkan::end_pass(
  context                   &ctx,
  cmd_end_render_pass const &info
) {}

} // namespace gzn::gfx::backends

#endif // defined(GZN_GFX_BACKEND_VULKAN)
