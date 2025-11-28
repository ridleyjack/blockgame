#include "VulkanRenderer.hpp"

#include "VulkanCommand.hpp"
#include "VulkanContext.hpp"
#include "VulkanDevice.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanPipelineCache.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanSync.hpp"

#include "Engine/GFX/RenderObject.hpp"
#include "Engine/GFX/Mesh.hpp"

namespace engine::gfx::vulkan {

Mesh gMesh{
    .Vertices{{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, //
              {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},  //
              {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}}  //
};

// ==============================
// Public Methods
// ==============================

Renderer::Renderer(GLFWwindow* window) {
  context_ = std::make_unique<Context>(window);
  renderObject_ = std::make_unique<gfx::RenderObject>(*context_);
}

Renderer::~Renderer() {
  renderObject_.reset();
  context_.reset();
}

void Renderer::Init() {
  context_->Init();
  renderObject_->Init();
  renderObject_->UploadMesh(gMesh);
}

void Renderer::DrawFrame() {
  auto& device = context_->GetDevice();
  auto& sync = context_->GetSync();
  auto& swapchain = context_->GetSwapchain();
  auto& cmd = context_->GetCommand();
  auto& pipeline = context_->GetPipelineLibrary().GetPipeline(0);

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
  vkResetFences(device.Logical(), 1, &sync.InFlightFence(currentFrame_));

  auto commandBuffer = cmd.Buffer(currentFrame_);
  vkResetCommandBuffer(commandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  VkExtent2D swapChainExtent = swapchain.Extent();

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = pipeline.RenderPass();
  renderPassInfo.framebuffer = swapchain.Framebuffer(imageIndex);
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GraphicsPipeline());

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swapChainExtent.width);
  viewport.height = static_cast<float>(swapChainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  renderObject_->Record(cmd.Buffer(currentFrame_), imageIndex, currentFrame_);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {sync.ImageAvailableSemaphore(currentFrame_)};
  VkSemaphore signalSemaphores[] = {sync.RenderFinishedSemaphore(imageIndex)};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd.Buffer(currentFrame_);

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

} // namespace engine::gfx::vulkan