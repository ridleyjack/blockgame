#include "FramebufferSet.hpp"

#include "Context.hpp"
#include "Device.hpp"
#include "RenderPassCache.hpp"
#include "SwapChain.hpp"

#include <cassert>
#include <stdexcept>

namespace engine::graphics::vulkan {

FramebufferSet::FramebufferSet(Context& context, const SwapChain& swapChain, const RenderPass& renderPass)
    : context_(context) {
  const auto vkDevice = context.GetDevice().Logical();
  const auto imageViews = swapChain.ImageViews();

  framebuffers_.resize(imageViews.size());
  for (size_t i = 0; i < imageViews.size(); i++) {
    const std::array attachments = {imageViews[i], swapChain.DepthImageView()};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass.Handle;
    framebufferInfo.attachmentCount = attachments.size();
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapChain.Extent().width;
    framebufferInfo.height = swapChain.Extent().height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(vkDevice, &framebufferInfo, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

FramebufferSet::~FramebufferSet() {
  const auto vkDevice = context_.GetDevice().Logical();
  for (const auto framebuffer : framebuffers_) {
    vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);
  }
}
VkFramebuffer FramebufferSet::Framebuffer(const uint32_t frameID) const noexcept {
  assert(frameID < framebuffers_.size());
  return framebuffers_[frameID];
}
} // namespace engine::graphics::vulkan
