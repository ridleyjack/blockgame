#include "VulkanRenderer.hpp"

#include "../TileGroup.hpp"
#include "VulkanCommand.hpp"
#include "VulkanContext.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanSync.hpp"

namespace Engine {

// ==============================
// Public Methods
// ==============================

Renderer::Renderer(GLFWwindow* window) {
  context_ = std::make_unique<Context>(window);
  tileGroup_ = std::make_unique<TileGroup>(*context_);
}

Renderer::~Renderer() {
  tileGroup_.reset();
  context_.reset();
}

void Renderer::Init() {
  context_->Init();
  tileGroup_->Init();
}

void Renderer::DrawFrame() {
  auto& device = context_->GetDevice();
  auto& sync = context_->GetSync();
  auto& swapchain = context_->GetSwapchain();
  auto& cmd = context_->GetCommand();

  vkWaitForFences(device.Logical(), 1, &sync.InFlightFence(currentFrame_), VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(device.Logical(), swapchain.Handle(), UINT64_MAX,
                                          sync.ImageAvailableSemaphore(currentFrame_), VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    swapchain.Recreate();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  tileGroup_->UpdateUniformBuffer(currentFrame_);

  vkResetFences(device.Logical(), 1, &sync.InFlightFence(currentFrame_));

  vkResetCommandBuffer(cmd.Buffer(currentFrame_), 0);
  tileGroup_->Record(cmd.Buffer(currentFrame_), imageIndex, currentFrame_);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {sync.ImageAvailableSemaphore(currentFrame_)};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd.Buffer(currentFrame_);
  VkSemaphore signalSemaphores[] = {sync.RenderFinishedSemaphore(currentFrame_)};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(device.GraphicsQueue(), 1, &submitInfo, sync.InFlightFence(currentFrame_)) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  VkSwapchainKHR swapChains[] = {swapchain.Handle()};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr; // Optional

  result = vkQueuePresentKHR(device.PresentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || context_->GetFramebufferResized()) {
    swapchain.Recreate();
    context_->SetFramebufferResized(false);
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  currentFrame_ = (currentFrame_ + 1) % config::MaxFramesInFlight;
}

bool Renderer::ShouldClose() const {
  return context_->WindowShouldClose();
}

void Renderer::WaitUntilIdle() const {
  return context_->WaitUntilIdle();
}

// ==============================
// Private Methods
// ==============================

} // namespace Engine