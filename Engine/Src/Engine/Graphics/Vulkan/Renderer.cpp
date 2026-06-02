#include "Renderer.hpp"

#include "Command.hpp"
#include "Context.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "PipelineCache.hpp"
#include "SwapChain.hpp"
#include "Sync.hpp"

#include "Engine/Graphics/CameraMatrices.hpp"
#include "Engine/Graphics/Handles.hpp"
#include "Engine/Graphics/Resources/TextureArrayBuilder.hpp"
#include "Engine/Graphics/PipelineCreateInfo.hpp"
#include "Engine/Assets/ImageLoader.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <cstring>
#include <array>

namespace engine::graphics::vulkan {

Renderer::Renderer(GLFWwindow* window)
    : context_(window),
      pipelineCache_(context_),
      stagingBuffer_(StagingBuffer::Create(context_)),
      uploader_(context_, stagingBuffer_),
      textureAllocator_(context_, uploader_, stagingBuffer_),
      meshBuffer_(MeshBuffer::Create(context_)),
      meshAllocator_(context_, uploader_, meshBuffer_, stagingBuffer_),
      descriptorAllocator_(context_, textureAllocator_) {

  const auto& device = context_.GetDevice();
  frameContext_.CameraGPU = DescriptorAllocator::CreateUniformBuffer(device);
  descriptorAllocator_.CreateGlobalDescriptorSets(frameContext_.CameraGPU);
}

Renderer::~Renderer() {
  context_.WaitUntilIdle();
  const auto& device = context_.GetDevice();
  for (const auto buffer : frameContext_.CameraGPU.Buffers)
    device.DestroyBuffer(buffer);
}

std::expected<void, RenderError> Renderer::BeginFrame(const CameraMatrices& camera) noexcept {
  const auto& device = context_.GetDevice();
  auto& sync = context_.GetSync();
  auto& swapchain = context_.GetSwapchain();
  const auto& cmd = context_.GetCommand();

  uploader_.Process();

  const VkExtent2D swapChainExtent = swapchain.Extent();

  vkWaitForFences(device.Logical(), 1, &sync.InFlightFence(frameContext_.CurrentFrame), VK_TRUE, UINT64_MAX);
  meshAllocator_.ProcessDeferredDeletions(frameContext_.CurrentFrame);

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

  // Camera
  const GlobalUBO ubo{.View = camera.View, .Projection = camera.Projection};
  memcpy(frameContext_.CameraGPU.MappedMemory[frameContext_.CurrentFrame], &ubo, sizeof(GlobalUBO));

  // Command Buffer
  const auto commandBuffer = cmd.PerFrameBuffer(frameContext_.CurrentFrame);
  vkResetCommandBuffer(commandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    return std::unexpected(RenderError::RecordCommandFailed);
  }

  frameContext_.FrameActive = true;

  std::array<VkImageMemoryBarrier2, 3> outputBarriers{
      VkImageMemoryBarrier2{
                            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                            .srcAccessMask = 0,
                            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                            .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                            .image = swapchain.ColorImage(),
                            .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}},
      VkImageMemoryBarrier2{
                            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                            .srcAccessMask = 0,
                            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                            .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                            .image = swapchain.Images()[frameContext_.ImageIndex],
                            .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}},
      VkImageMemoryBarrier2{
                            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                            .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                            .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                            .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                            .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                            .image = swapchain.DepthImage(),
                            .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1}}
  };
  VkDependencyInfo barrierDependencyInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                         .imageMemoryBarrierCount = outputBarriers.size(),
                                         .pImageMemoryBarriers = outputBarriers.data()};
  vkCmdPipelineBarrier2(commandBuffer, &barrierDependencyInfo);

  VkRenderingAttachmentInfo colorAttachment{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                                            .imageView = swapchain.ColorImageView(),
                                            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                                            .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
                                            .resolveImageView = swapchain.ImageViews()[frameContext_.ImageIndex],
                                            .resolveImageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                                            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                            .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}};
  VkRenderingAttachmentInfo depthAttachment{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                                            .imageView = swapchain.DepthImageView(),
                                            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                                            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                            .clearValue = {.depthStencil = {1.0f, 0}}};
  VkRenderingInfo renderInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = VkRect2D{VkOffset2D{}, swapchain.Extent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
      .pDepthAttachment = &depthAttachment,
  };
  vkCmdBeginRendering(commandBuffer, &renderInfo);

  VkViewport viewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(swapChainExtent.width),
      .height = static_cast<float>(swapChainExtent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{
      .offset = {0, 0},
      .extent = swapChainExtent,
  };
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  return {};
}

std::expected<void, RenderError> Renderer::EndFrame() {
  const auto& device = context_.GetDevice();
  auto& sync = context_.GetSync();
  auto& swapchain = context_.GetSwapchain();

  const auto& cmd = context_.GetCommand();
  const auto commandBuffer = cmd.PerFrameBuffer(frameContext_.CurrentFrame);

  vkCmdEndRendering(commandBuffer);
  frameContext_.FrameActive = false;

  VkImageMemoryBarrier2 barrierPresent{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = 0,
      .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .image = swapchain.Images()[frameContext_.ImageIndex],
      .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}
  };
  VkDependencyInfo barrierPresentDependencyInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                                .imageMemoryBarrierCount = 1,
                                                .pImageMemoryBarriers = &barrierPresent};
  vkCmdPipelineBarrier2(commandBuffer, &barrierPresentDependencyInfo);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    return std::unexpected(RenderError::RecordCommandFailed);
  }

  const std::array<VkSemaphore, 1> waitSemaphores = {sync.ImageAvailableSemaphore(frameContext_.CurrentFrame)};
  const std::array<VkSemaphore, 1> signalSemaphores = {sync.RenderFinishedSemaphore(frameContext_.ImageIndex)};
  constexpr std::array<VkPipelineStageFlags, 1> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = waitSemaphores.data(),
      .pWaitDstStageMask = waitStages.data(),
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer,

      .signalSemaphoreCount = 1,
      .pSignalSemaphores = signalSemaphores.data(),
  };

  vkResetFences(device.Logical(), 1, &sync.InFlightFence(frameContext_.CurrentFrame));
  if (vkQueueSubmit(device.GraphicsQueue(), 1, &submitInfo, sync.InFlightFence(frameContext_.CurrentFrame)) !=
      VK_SUCCESS) {
    // TODO: Fence is never signalled on this path. Fence needs to be recreated or error simply treated as fatal.
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

void Renderer::Submit(const PipelineHandle& pipelineHandle,
                      const MeshHandle& handle,
                      const MaterialHandle& material,
                      const ObjectPushConstants& model) {
  const auto& cmd = context_.GetCommand();
  const auto commandBuffer = cmd.PerFrameBuffer(frameContext_.CurrentFrame);

  const MeshAllocator::MeshGPU& meshGPU = meshAllocator_.Get(handle.MeshID);
  if (meshGPU.State != MeshState::Ready)
    return;

  const auto globalDescriptor = descriptorAllocator_.GlobalDescriptorSet(frameContext_.CurrentFrame);
  const auto texDescriptor = descriptorAllocator_.TextureDescriptorSet(material.DescriptorSetID);
  if (texDescriptor == VK_NULL_HANDLE)
    return;

  const auto& pipeline = pipelineCache_.GetPipeline(pipelineHandle.PipelineID);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Handle());

  const std::array<VkBuffer, 1> vertexBuffers = {meshBuffer_.Handle()};
  const std::array<VkDeviceSize, 1> offsets = {meshGPU.VertexOffset};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
  vkCmdBindIndexBuffer(commandBuffer, meshBuffer_.Handle(), meshGPU.IndexOffset, VK_INDEX_TYPE_UINT32);

  const std::array<VkDescriptorSet, 2> descriptors{globalDescriptor, texDescriptor};
  vkCmdBindDescriptorSets(commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.PipelineLayout(),
                          0,
                          descriptors.size(),
                          descriptors.data(),
                          0,
                          nullptr);

  vkCmdPushConstants(commandBuffer,
                     pipeline.PipelineLayout(),
                     VK_SHADER_STAGE_VERTEX_BIT,
                     0,
                     sizeof(ObjectPushConstants),
                     &model);

  vkCmdDrawIndexed(commandBuffer, meshGPU.IndexCount, 1, 0, 0, 0);
}

bool Renderer::ShouldClose() const noexcept {
  return context_.WindowShouldClose();
}

void Renderer::WaitUntilIdle() const {
  context_.WaitUntilIdle();
}

PipelineHandle Renderer::CreatePipeline(const PipelineCreateInfo& info) {
  auto pipeline = pipelineCache_.CreatePipeline(info, descriptorAllocator_.DescriptorSetLayouts());
  if (!pipeline) {
    const std::string msg = std::format("Failed to create render pass: {}", ToString(pipeline.error()));
    throw std::runtime_error(msg);
  }

  return PipelineHandle{*pipeline};
}

void Renderer::DeletePipeline(const PipelineHandle& handle) {
  pipelineCache_.DestroyPipeline(handle.PipelineID);
}

MeshHandle Renderer::CreateMesh(const Mesh& mesh) {
  auto result = meshAllocator_.Create(mesh);
  if (!result) {
    throw std::runtime_error(std::format("Failed to create mesh: {}", ToString(result.error())));
  }
  return MeshHandle{.MeshID = *result};
}

void Renderer::DeleteMesh(const MeshHandle& handle) {
  std::uint32_t retireFrame = frameContext_.CurrentFrame;
  if (!frameContext_.FrameActive) // Retire after last frame is done.
    retireFrame = (frameContext_.CurrentFrame + config::MaxFramesInFlight - 1) % config::MaxFramesInFlight;

  meshAllocator_.DeleteDeferred(handle.MeshID, retireFrame);
}

std::expected<TextureHandle, TextureError>
Renderer::CreateTexture(const std::span<const std::byte>& data, const int width, const int height) {
  const auto result = textureAllocator_.Create(data, width, height);
  if (!result) {
    return std::unexpected(result.error());
  }
  return TextureHandle{.TextureID = *result};
}

std::expected<resources::TextureArrayBuilder, TextureError>
Renderer::BeginTextureArray(const resources::TextureArrayInfo& info) noexcept {
  if (auto result = textureAllocator_.BeginArray(info); !result)
    return std::unexpected{result.error()};

  return resources::TextureArrayBuilder{textureAllocator_};
}

MaterialHandle Renderer::CreateMaterial(const TextureHandle& texture) {
  const std::uint32_t descriptorID = descriptorAllocator_.CreateTextureDescriptorSet(texture.TextureID);
  return MaterialHandle{.TextureID = texture.TextureID, .DescriptorSetID = descriptorID};
}

glm::mat4 Renderer::MakeProjection(const ProjectionSettings& settings) const noexcept {
  const auto extent = context_.GetSwapchain().Extent();
  const float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
  auto proj = glm::perspectiveRH_ZO(glm::radians(settings.FOV), aspect, settings.NearPlane, settings.FarPlane);
  proj[1][1] *= -1;
  return proj;
}

void Renderer::SetFramebufferResized(const bool hasResized) noexcept {
  context_.SetFramebufferHasResized(hasResized);
}
} // namespace engine::graphics::vulkan