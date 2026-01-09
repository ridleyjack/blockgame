#include "Renderer.hpp"

#include "Command.hpp"
#include "Context.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "PipelineCache.hpp"
#include "SwapChain.hpp"
#include "Sync.hpp"
#include "RenderPassCache.hpp"
#include "Engine/Graphics/Camera.h"

#include "Engine/Graphics/Handles.hpp"
#include "Engine/Graphics/PipelineCreateInfo.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <Engine/Assets/ImageLoader.hpp>

#include <cstring>
#include <array>
#include <print>

// Single resources and hardcoded values are used in this stage of development.
constexpr uint32_t defaultRenderPassID = 0;
constexpr uint32_t defaultFramebufferID = 0;
constexpr uint32_t defaultPipelineID = 0;

namespace engine::graphics::vulkan {
Renderer::Renderer(GLFWwindow* window)
    : context_(window),
      renderPassCache_(context_),
      pipelineCache_(context_),
      descriptorAllocator_(context_),
      textureAllocator_(context_),
      meshAllocator_(context_) {
  const auto& device = context_.GetDevice();

  // Init hardcoded render pass at index 0;.
  if (const auto result = renderPassCache_.CreateRenderPass(); !result)
    throw std::runtime_error("Failed to create Render Pass");
  RenderPass& renderPass = renderPassCache_.GetRenderPass(defaultRenderPassID);

  // Init hardcoded framebufferSet at index 0;.
  context_.GetSwapchain().CreateFramebuffers(renderPass);

  // Init hardcoded pipeline at index 0;.
  const PipelineCreateInfo info{.RenderPass = RenderPassHandle{defaultRenderPassID},
                                .VertexShaderFile = "Shaders/vert.spv",
                                .FragmentShaderFile = "Shaders/frag.spv"};
  pipelineCache_.CreatePipeline(info, renderPass);

  auto loader = assets::ImageLoader{};
  if (auto result = loader.Load("Textures/texture.jpg"); !result) {
    std::println("Failed to load texture: {}", result.error());
  }
  const auto handle = CreateTexture(loader.Data(), loader.Width(), loader.Height());
  const auto& texture = textureAllocator_.Get(handle.TextureID);

  frameContext_.CameraGPU = DescriptorAllocator::CreateUniformBuffer(device);
  descriptorAllocator_.CreateDescriptorSet(pipelineCache_.DescriptorSetLayout(),
                                           frameContext_.CameraGPU,
                                           texture.ImageView,
                                           texture.Sampler);
}

Renderer::~Renderer() {
  const auto& device = context_.GetDevice();

  for (const auto buffer : frameContext_.CameraGPU.Buffers)
    device.DestroyBuffer(buffer);
}

std::expected<void, RenderError> Renderer::BeginFrame(const RenderPassHandle& renderPassHandle,
                                                      const PipelineHandle& pipelineHandle,
                                                      const Camera& camera) noexcept {
  const auto& device = context_.GetDevice();
  auto& sync = context_.GetSync();
  auto& swapchain = context_.GetSwapchain();
  const auto& cmd = context_.GetCommand();

  const VkExtent2D swapChainExtent = swapchain.Extent();

  frameContext_.RenderPassID = renderPassHandle.RenderPassID;
  frameContext_.PipelineID = pipelineHandle.PipelineID;

  VkRenderPass renderPass = renderPassCache_.GetRenderPass(frameContext_.RenderPassID).Handle;
  VkPipeline pipeline = pipelineCache_.GetPipeline(frameContext_.PipelineID).Handle();

  vkWaitForFences(device.Logical(), 1, &sync.InFlightFence(frameContext_.CurrentFrame), VK_TRUE, UINT64_MAX);
  const VkResult result = vkAcquireNextImageKHR(device.Logical(),
                                                swapchain.Handle(),
                                                UINT64_MAX,
                                                sync.ImageAvailableSemaphore(frameContext_.CurrentFrame),
                                                VK_NULL_HANDLE,
                                                &frameContext_.ImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    swapchain.Recreate();
    return std::unexpected(RenderError::FrameOutOfDate);
  }
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    return std::unexpected(RenderError::FrameAcquireFailed);
  }
  vkResetFences(device.Logical(), 1, &sync.InFlightFence(frameContext_.CurrentFrame));

  // Update hardcoded UniformBuffer.
  // UniformBufferObject ubo{
  //     .Model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
  //     .View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
  //     .Projection =
  //         glm::perspective(glm::radians(45.0f),
  //                          static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height),
  //                          0.1f,
  //                          10.0f)};

  // Camera
  UniformBufferObject ubo{.Model = glm::mat4(1.0f),
                          .View = camera.View(),
                          .Projection = camera.Projection(swapchain.Extent().width, swapchain.Extent().height)};

  memcpy(frameContext_.CameraGPU.MappedMemory[frameContext_.CurrentFrame], &ubo, sizeof(UniformBufferObject));

  // Command Buffer
  const auto commandBuffer = cmd.Buffer(frameContext_.CurrentFrame);
  vkResetCommandBuffer(commandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    return std::unexpected(RenderError::RecordCommandFailed);
  }

  // Render Pass
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapchain.Framebuffers(defaultFramebufferID).Framebuffer(frameContext_.ImageIndex);
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
  clearValues[1].depthStencil = {1.0f, 0};

  renderPassInfo.clearValueCount = clearValues.size();
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

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

  return {};
}

std::expected<void, RenderError> Renderer::EndFrame() {
  const auto& device = context_.GetDevice();
  auto& sync = context_.GetSync();
  auto& swapchain = context_.GetSwapchain();

  const auto& cmd = context_.GetCommand();
  const auto commandBuffer = cmd.Buffer(frameContext_.CurrentFrame);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    return std::unexpected(RenderError::RecordCommandFailed);
  }

  const std::array<VkSemaphore, 1> waitSemaphores = {sync.ImageAvailableSemaphore(frameContext_.CurrentFrame)};
  const std::array<VkSemaphore, 1> signalSemaphores = {sync.RenderFinishedSemaphore(frameContext_.ImageIndex)};
  constexpr std::array<VkPipelineStageFlags, 1> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores.data();
  submitInfo.pWaitDstStageMask = waitStages.data();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores.data();

  if (vkQueueSubmit(device.GraphicsQueue(), 1, &submitInfo, sync.InFlightFence(frameContext_.CurrentFrame)) !=
      VK_SUCCESS) {
    return std::unexpected(RenderError::SubmitCommandFailed);
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores.data();
  const std::array<VkSwapchainKHR, 1> swapChains = {swapchain.Handle()};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains.data();
  presentInfo.pImageIndices = &frameContext_.ImageIndex;
  presentInfo.pResults = nullptr; // Optional

  const VkResult result = vkQueuePresentKHR(device.PresentQueue(), &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || context_.GetFramebufferHasResized()) {
    swapchain.Recreate();
    context_.SetFramebufferHasResized(false);
  } else if (result != VK_SUCCESS) {
    return std::unexpected(RenderError::PresentFailed);
  }
  frameContext_.CurrentFrame = (frameContext_.CurrentFrame + 1) % config::MaxFramesInFlight;

  return {};
}

void Renderer::Submit(const MeshHandle& handle) {
  const auto& cmd = context_.GetCommand();
  const auto commandBuffer = cmd.Buffer(frameContext_.CurrentFrame);

  const MeshGPU& gpuMesh = meshAllocator_.Get(handle.MeshID);
  const std::array<VkBuffer, 1> vertexBuffers = {gpuMesh.VertexBuffer.Buffer};
  constexpr std::array<VkDeviceSize, 1> offsets = {0};

  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
  vkCmdBindIndexBuffer(commandBuffer, gpuMesh.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT16);

  auto& pipeline = pipelineCache_.GetPipeline(frameContext_.PipelineID);
  auto descriptor = descriptorAllocator_.DescriptorSet(frameContext_.CurrentFrame);
  vkCmdBindDescriptorSets(commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.PipelineLayout(),
                          0,
                          1,
                          &descriptor,
                          0,
                          nullptr);
  vkCmdDrawIndexed(commandBuffer, gpuMesh.IndexCount, 1, 0, 0, 0);
}

bool Renderer::ShouldClose() const noexcept {
  return context_.WindowShouldClose();
}

void Renderer::WaitUntilIdle() const {
  context_.WaitUntilIdle();
}
RenderPassHandle Renderer::CreateRenderPass() {
  if (auto result = renderPassCache_.CreateRenderPass(); !result) {
    const std::string msg = std::format("Failed to create render pass: {}", ToString(result.error()));
    throw std::runtime_error(msg);
  } else {
    return RenderPassHandle{*result};
  }
}

PipelineHandle Renderer::CreatePipeline(const PipelineCreateInfo& info) {
  RenderPass& pass = renderPassCache_.GetRenderPass(info.RenderPass.RenderPassID);
  return pipelineCache_.CreatePipeline(info, pass);
}

void Renderer::DeletePipeline(const PipelineHandle handle) {
  pipelineCache_.DestroyPipeline(handle.PipelineID);
}

MeshHandle Renderer::CreateMesh(const Mesh& mesh) {
  const uint32_t meshID = meshAllocator_.Create(mesh);
  return MeshHandle{.MeshID = meshID};
}

void Renderer::DeleteMesh(const MeshHandle handle) {
  meshAllocator_.Delete(handle.MeshID);
}

TextureHandle Renderer::CreateTexture(const std::span<const std::byte> data, const int width, const int height) {
  const auto result = textureAllocator_.Create(data, width, height);
  if (!result) {
    throw std::runtime_error("Failed to create texture");
  }
  return TextureHandle{.TextureID = *result};
}
} // namespace engine::graphics::vulkan