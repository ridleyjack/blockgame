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
  auto device = context_.GetDevice().Logical();

  this->DestroyPerImageSemaphores();
  for (size_t i = 0; i < config::MaxFramesInFlight; i++) {
    vkDestroySemaphore(device, imageAvailableSemaphores_[i], nullptr);
    vkDestroyFence(device, inFlightFences_[i], nullptr);
  }
}

void Sync::Init() {
  auto device = context_.GetDevice().Logical();

  imageAvailableSemaphores_.resize(config::MaxFramesInFlight);
  inFlightFences_.resize(config::MaxFramesInFlight);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  this->CreatePerImageSemaphores();

  for (size_t i = 0; i < config::MaxFramesInFlight; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create semaphores!");
    }
  }
}

void Sync::CreatePerImageSemaphores() {
  auto device = context_.GetDevice().Logical();
  auto images = context_.GetSwapchain().Images();

  this->DestroyPerImageSemaphores();
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  renderFinishedSemaphores_.resize(images.size());
  for (size_t i = 0; i < images.size(); i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create render finished semaphores!");
    }
  }
}

void Sync::DestroyPerImageSemaphores() {
  auto device = context_.GetDevice().Logical();

  for (const auto& m_renderFinishedSemaphore : renderFinishedSemaphores_) {
    vkDestroySemaphore(device, m_renderFinishedSemaphore, nullptr);
  }
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

} // namespace Engine::GFX::Vulkan
