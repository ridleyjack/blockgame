#include "RenderPassCache.hpp"

#include "Context.hpp"
#include "Device.hpp"
#include "SwapChain.hpp"

#include <cassert>

namespace engine::graphics::vulkan {

RenderPassCache::RenderPassCache(Context& context) : context_(context) {}

RenderPassCache::~RenderPassCache() {
  const auto vkDevice = context_.GetDevice().Logical();

  for (auto pass : renderPasses_) {
    vkDestroyRenderPass(vkDevice, pass.Handle, nullptr);
  }
}

std::expected<uint32_t, RenderPassError> RenderPassCache::CreateRenderPass() {
  if (auto r = createRenderPass_(); !r) {
    return std::unexpected(r.error());
  } else {
    renderPasses_.push_back(*r);
    return renderPasses_.size() - 1;
  }
}

RenderPass& RenderPassCache::GetRenderPass(const uint32_t renderPassID) noexcept {
  assert(renderPassID < renderPasses_.size());
  return renderPasses_[renderPassID];
}

std::expected<RenderPass, RenderPassError> RenderPassCache::createRenderPass_() const noexcept {
  const auto& swapchain = context_.GetSwapchain();
  const auto& device = context_.GetDevice();

  VkAttachmentDescription colorAttachment{
      .format = swapchain.ImageFormat(),
      .samples = device.MsaaSamples(),
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkAttachmentReference colorAttachmentRef{
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkAttachmentDescription depthAttachment{
      .format = findDepthFormat_(),
      .samples = device.MsaaSamples(),
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  VkAttachmentReference depthAttachmentRef{
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  VkAttachmentDescription colorAttachmentResolve{
      .format = swapchain.ImageFormat(),
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkAttachmentReference colorAttachmentResolveRef{
      .attachment = 2,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass{
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentRef,
      .pResolveAttachments = &colorAttachmentResolveRef,
      .pDepthStencilAttachment = &depthAttachmentRef,
  };

  VkSubpassDependency dependency{
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
  };

  const std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
  VkRenderPassCreateInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency,
  };

  VkRenderPass renderPass{VK_NULL_HANDLE};
  if (vkCreateRenderPass(device.Logical(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
    return std::unexpected(RenderPassError::FailedToCreateRenderPass);
  }
  return RenderPass{.Handle = renderPass};
}

VkFormat RenderPassCache::findDepthFormat_() const {
  return context_.GetDevice().FindSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

} // namespace engine::graphics::vulkan
