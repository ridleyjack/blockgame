#include "Sync.hpp"

#include "Context.hpp"
#include "Device.hpp"
#include "SwapChain.hpp"

#include <cassert>

namespace engine::graphics::vulkan {

Sync::Sync(Context& context) : context_(context) {
  const auto device = context_.GetDevice().Logical();

  imageAvailableSemaphores_.resize(config::MaxFramesInFlight);
  inFlightFences_.resize(config::MaxFramesInFlight);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < config::MaxFramesInFlight; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create semaphores!");
    }
  }
  createPerImageSemaphores_();
}

Sync::~Sync() {
  const auto vkDevice = context_.GetDevice().Logical();
  for (const auto semaphore : imageAvailableSemaphores_) {
    vkDestroySemaphore(vkDevice, semaphore, nullptr);
  }
  for (const auto fence : inFlightFences_) {
    vkDestroyFence(vkDevice, fence, nullptr);
  }
  cleanupPerImageSemaphores_();
}

void Sync::RecreatePerImageSemaphores() {
  cleanupPerImageSemaphores_();
  createPerImageSemaphores_();
}

VkSemaphore& Sync::ImageAvailableSemaphore(const uint32_t frame) noexcept {
  assert(frame < imageAvailableSemaphores_.size());
  return imageAvailableSemaphores_[frame];
}

VkSemaphore& Sync::RenderFinishedSemaphore(const uint32_t image) noexcept {
  assert(image < renderFinishedSemaphores_.size());
  return renderFinishedSemaphores_[image];
}

VkFence& Sync::InFlightFence(const uint32_t frame) noexcept {
  assert(frame < inFlightFences_.size());
  return inFlightFences_[frame];
}

void Sync::createPerImageSemaphores_() {
  const auto vkDevice = context_.GetDevice().Logical();
  const size_t numImages = context_.GetSwapchain().Images().size();

  renderFinishedSemaphores_.resize(numImages);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  for (size_t i = 0; i < numImages; i++) {
    if (vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create semaphores!");
  }
}

void Sync::cleanupPerImageSemaphores_() {
  const auto vkDevice = context_.GetDevice().Logical();
  for (const auto semaphore : renderFinishedSemaphores_)
    vkDestroySemaphore(vkDevice, semaphore, nullptr);
}

} // namespace engine::graphics::vulkan
