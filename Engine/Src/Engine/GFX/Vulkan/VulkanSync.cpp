#include "VulkanSync.hpp"

#include "VulkanContext.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"

namespace engine::gfx::vulkan {

// ==============================
// Public Methods
// ==============================

Sync::Sync(Context& context) : context_(context) {}

Sync::~Sync() {
  const auto vkDevice = context_.GetDevice().Logical();
  for (size_t i = 0; i < config::MaxFramesInFlight; i++) {
    vkDestroySemaphore(vkDevice, imageAvailableSemaphores_[i], nullptr);
    vkDestroyFence(vkDevice, inFlightFences_[i], nullptr);
  }
  cleanupPerImageSemaphores_();
}

void Sync::Init() {
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
void Sync::RecreatePerImageSemaphores() {
  cleanupPerImageSemaphores_();
  createPerImageSemaphores_();
}

VkSemaphore& Sync::ImageAvailableSemaphore(uint32_t frame) {
  return imageAvailableSemaphores_[frame];
}

VkSemaphore& Sync::RenderFinishedSemaphore(uint32_t frame) {
  return renderFinishedSemaphores_[frame];
}

VkFence& Sync::InFlightFence(uint32_t frame) {
  return inFlightFences_[frame];
}

// ==============================
// Private Methods
// ==============================

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

} // namespace engine::gfx::vulkan
