#include "Renderer.hpp"

#include "Command.hpp"
#include "CheckVk.hpp"
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
#include "Engine/Fatal.hpp"
#include "Engine/Graphics/SubmitInfo.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <cstring>
#include <array>
#include <span>

namespace engine::graphics::vulkan {

Renderer::Renderer(GLFWwindow* window)
    : context_(window),
      pipelineCache_(context_),
      stagingBuffer_(StagingBuffer::Create(context_)),
      uploader_(context_, stagingBuffer_),
      textureAllocator_(context_, uploader_, stagingBuffer_),
      meshBuffer_(MeshBuffer::Create(context_)),
      meshAllocator_(context_, uploader_, meshBuffer_, stagingBuffer_),
      descriptorAllocator_(context_, textureAllocator_),
      shaderDataAllocator_(context_) {

  const auto& device = context_.GetDevice();
  cameraShaderDataID_ = shaderDataAllocator_.AllocateShaderData(sizeof(CameraShaderData));
}

Renderer::~Renderer() {
  context_.WaitUntilIdle();
}

std::expected<void, RenderError> Renderer::BeginFrame(const CameraMatrices& camera) {
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
    CheckVk(result, "vkAcquireNextImageKHR");
  }

  // Camera
  const CameraShaderData ubo{.View = camera.View, .Projection = camera.Projection};
  shaderDataAllocator_.WriteShaderData(cameraShaderDataID_,
                                       frameContext_.CurrentFrame,
                                       std::as_bytes(std::span{&ubo, 1}));

  // Command Buffer
  const auto commandBuffer = cmd.PerFrameBuffer(frameContext_.CurrentFrame);
  CheckVk(vkResetCommandBuffer(commandBuffer, 0), "vkResetCommandBuffer");

  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  CheckVk(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer");

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

void Renderer::EndFrame() {
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

  CheckVk(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer");

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

  CheckVk(vkResetFences(device.Logical(), 1, &sync.InFlightFence(frameContext_.CurrentFrame)), "vkResetFences");
  CheckVk(vkQueueSubmit(device.GraphicsQueue(), 1, &submitInfo, sync.InFlightFence(frameContext_.CurrentFrame)),
          "vkQueueSubmit");

  const std::array<VkSwapchainKHR, 1> swapChains = {swapchain.Handle()};
  VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = signalSemaphores.data(),
      .swapchainCount = 1,
      .pSwapchains = swapChains.data(),
      .pImageIndices = &frameContext_.ImageIndex,
      .pResults = nullptr,
  };

  const VkResult result = vkQueuePresentKHR(device.PresentQueue(), &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || context_.GetFramebufferHasResized()) {
    swapchain.Recreate();
    context_.SetFramebufferHasResized(false);
  } else if (result != VK_SUCCESS) {
    CheckVk(result, "vkQueuePresentKHR");
  }

  frameContext_.CurrentFrame = (frameContext_.CurrentFrame + 1) % config::MaxFramesInFlight;
}

void Renderer::Submit(const SubmitInfo& info) {
  const auto& cmd = context_.GetCommand();
  const auto commandBuffer = cmd.PerFrameBuffer(frameContext_.CurrentFrame);

  const auto& pipeline = pipelineCache_.GetPipeline(info.Pipeline.PipelineID);

  const MeshAllocator::MeshGPU& meshGPU = meshAllocator_.Get(info.Mesh.MeshID);
  if (meshGPU.State != MeshState::Ready)
    return;

  std::array<VkDescriptorSet, 1> descriptors{VK_NULL_HANDLE};
  std::uint32_t numDescriptorSets = 0;

  if (pipeline.Kind() == PipelineKind::SolidTexture) {
    const auto texDescriptor = descriptorAllocator_.TextureDescriptorSet(info.Material.DescriptorSetID);
    if (texDescriptor == VK_NULL_HANDLE)
      return;
    descriptors[numDescriptorSets++] = texDescriptor;
  }

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Handle());

  const std::array<VkBuffer, 1> vertexBuffers = {meshBuffer_.Handle()};
  const std::array<VkDeviceSize, 1> offsets = {meshGPU.VertexOffset};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
  vkCmdBindIndexBuffer(commandBuffer, meshBuffer_.Handle(), meshGPU.IndexOffset, VK_INDEX_TYPE_UINT32);

  if (numDescriptorSets > 0)
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.PipelineLayout(),
                            0,
                            numDescriptorSets,
                            descriptors.data(),
                            0,
                            nullptr);

  ObjectPushConstants pushConstants = {
      .CameraAddress = shaderDataAllocator_.GetShaderDataAddress(cameraShaderDataID_, frameContext_.CurrentFrame),
  };

  if (info.ShaderData.size() > static_cast<size_t>(ObjectPushConstants::MaxShaderDataSlots))
    Fatal("Renderer Submit called with shaderDataSlots exceeding MaxShaderDataSlots");
  if (!pipeline.ValidShaderData(info.ShaderData))
    Fatal("Renderer Submit called with shaderData that is not compatible with bound pipeline");

  for (std::size_t i = 0; i < info.ShaderData.size(); i++) {
    VkDeviceAddress addr =
        shaderDataAllocator_.GetShaderDataAddress(info.ShaderData[i].ShaderDataID, frameContext_.CurrentFrame);
    pushConstants.ShaderData[i] = addr;
  }

  vkCmdPushConstants(commandBuffer,
                     pipeline.PipelineLayout(),
                     VK_SHADER_STAGE_VERTEX_BIT,
                     0,
                     sizeof(ObjectPushConstants),
                     &pushConstants);

  vkCmdDrawIndexed(commandBuffer, meshGPU.IndexCount, 1, 0, 0, 0);
} // namespace engine::graphics::vulkan

bool Renderer::ShouldClose() const noexcept {
  return context_.WindowShouldClose();
}

void Renderer::WaitUntilIdle() const {
  context_.WaitUntilIdle();
}

PipelineHandle Renderer::CreatePipeline(const PipelineCreateInfo& info) {
  std::span<const VkDescriptorSetLayout> layouts{};
  if (info.Kind == PipelineKind::SolidTexture) {
    layouts = descriptorAllocator_.TextureSetLayout();
  }

  auto pipeline = pipelineCache_.CreatePipeline(info, layouts);
  if (!pipeline) {
    const std::string msg = std::format("Failed to create pipeline: {}", ToString(pipeline.error()));
    Fatal(msg);
  }

  return PipelineHandle{*pipeline};
}

void Renderer::DeletePipeline(const PipelineHandle handle) {
  pipelineCache_.DestroyPipeline(handle.PipelineID);
}

MeshHandle Renderer::CreateMesh(const Mesh& mesh) {
  const auto result = meshAllocator_.Create(mesh);
  return MeshHandle{.MeshID = result};
}

void Renderer::DeleteMesh(const MeshHandle handle) {
  std::uint32_t retireFrame = frameContext_.CurrentFrame;
  if (!frameContext_.FrameActive) // Retire after last frame is done.
    retireFrame = (frameContext_.CurrentFrame + config::MaxFramesInFlight - 1) % config::MaxFramesInFlight;

  meshAllocator_.DeleteDeferred(handle.MeshID, retireFrame);
}

bool Renderer::IsMeshReady(const MeshHandle mesh) noexcept {
  return meshAllocator_.Get(mesh.MeshID).State == MeshState::Ready;
}

TextureHandle Renderer::CreateTexture(const std::span<const std::byte>& data, const int width, const int height) {
  const auto result = textureAllocator_.Create(data, width, height);
  return TextureHandle{.TextureID = result};
}

resources::TextureArrayBuilder Renderer::BeginTextureArray(const resources::TextureArrayInfo& info) {
  textureAllocator_.BeginArray(info);
  return resources::TextureArrayBuilder{textureAllocator_};
}

MaterialHandle Renderer::CreateMaterial(const TextureHandle texture) {
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
