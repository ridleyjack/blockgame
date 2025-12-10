#include "Renderer.hpp"

#include "Command.hpp"
#include "Context.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "PipelineCache.hpp"
#include "SwapChain.hpp"
#include "Sync.hpp"
#include "RenderPass.hpp"

#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Handles.hpp"
#include "Engine/Graphics/PipelineCreateInfo.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <cstring>
#include <array>

// Single resources and hardcoded values are used in this stage of development.
constexpr uint32_t defaultRenderPassID = 0;
constexpr uint32_t defaultFramebufferID = 0;
constexpr uint32_t defaultPipelineID = 0;
constexpr uint32_t defaultUniformID = 0;

namespace engine::graphics::vulkan {
Renderer::Renderer(GLFWwindow* window) : context_(window), pipelineCache_(context_) {
  const auto& device = context_.GetDevice();

  // Init hardcoded render pass at index 0;.
  renderPasses_.Create(context_);

  // Init hardcoded framebuffer at index 0;.
  framebuffers_.Create(context_, context_.GetSwapchain(), renderPasses_.Get(defaultRenderPassID));

  // Init hardcoded pipeline at index 0;.
  const PipelineCreateInfo info{"Shaders/vert.spv", "Shaders/frag.spv"};
  pipelineCache_.CreatePipeline(info, renderPasses_.Get(defaultRenderPassID));

  // Init hardcoded UniformBuffer at index 0;

  uniforms_.Create();
  UniformBuffer& uniform = uniforms_.Get(defaultUniformID);
  for (size_t i = 0; i < config::MaxFramesInFlight; i++) {
    constexpr VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniform.Buffers[i] =
        device.CreateBuffer(bufferSize,
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkMapMemory(device.Logical(), uniform.Buffers[i].Memory, 0, bufferSize, 0, &uniform.MappedMemory[i]);
  }

  // Init Descriptor Pool.
  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = static_cast<uint32_t>(config::MaxFramesInFlight);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = static_cast<uint32_t>(config::MaxFramesInFlight);
  if (vkCreateDescriptorPool(device.Logical(), &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }

  // Create Descriptor Sets.
  const VkDescriptorSetLayout layout =
      pipelineCache_.GetPipeline(PipelineHandle{defaultPipelineID}).DescriptorSetLayout();
  std::array<VkDescriptorSetLayout, config::MaxFramesInFlight> layouts{};
  layouts.fill(layout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool_;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(config::MaxFramesInFlight);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets_.resize(config::MaxFramesInFlight);
  if (vkAllocateDescriptorSets(device.Logical(), &allocInfo, descriptorSets_.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  // Update Descriptor Sets.
  for (size_t i = 0; i < config::MaxFramesInFlight; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniform.Buffers[i].Buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSets_[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(device.Logical(), 1, &descriptorWrite, 0, nullptr);
  }
}

Renderer::~Renderer() {
  const auto& device = context_.GetDevice();

  vkDestroyDescriptorPool(device.Logical(), descriptorPool_, nullptr);

  // Destroy UniformBuffers.
  for (size_t i = 0; i < uniforms_.Size(); i++) {
    if (!meshes_.Contains(i)) {
      continue;
    }

    UniformBuffer& uniform = uniforms_.Get(i);
    for (auto buffer : uniform.Buffers) {
      device.DestroyBuffer(buffer);
    }
  }

  // Destroy Meshes.
  for (size_t i = 0; i < meshes_.Size(); i++) {
    if (meshes_.Contains(i))
      this->DeleteMesh({.MeshID = static_cast<uint32_t>(i)});
  }
}

std::expected<FrameContext, RenderError> Renderer::BeginFrame() noexcept {
  const auto& device = context_.GetDevice();
  auto& sync = context_.GetSync();
  auto& swapchain = context_.GetSwapchain();
  const auto& cmd = context_.GetCommand();

  uint32_t imageID;
  vkWaitForFences(device.Logical(), 1, &sync.InFlightFence(currentFrame_), VK_TRUE, UINT64_MAX);
  const VkResult result = vkAcquireNextImageKHR(device.Logical(),
                                                swapchain.Handle(),
                                                UINT64_MAX,
                                                sync.ImageAvailableSemaphore(currentFrame_),
                                                VK_NULL_HANDLE,
                                                &imageID);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    swapchain.Recreate();
    return std::unexpected(RenderError::FrameOutOfDate);
  }
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    return std::unexpected(RenderError::FrameAcquireFailed);
  }
  vkResetFences(device.Logical(), 1, &sync.InFlightFence(currentFrame_));

  // Update hardcoded UniformBuffer.
  const UniformBufferObject ubo{.Model =
                                    glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f))};
  memcpy(uniforms_.Get(defaultUniformID).MappedMemory[currentFrame_], &ubo, sizeof(UniformBufferObject));

  // Command Buffer
  const auto commandBuffer = cmd.Buffer(currentFrame_);
  vkResetCommandBuffer(commandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    return std::unexpected(RenderError::RecordCommandFailed);
  }

  // Render Pass
  const auto& pipeline = pipelineCache_.GetPipeline(PipelineHandle{defaultPipelineID});
  const VkExtent2D swapChainExtent = swapchain.Extent();

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPasses_.Get(defaultRenderPassID).Handle();
  renderPassInfo.framebuffer = framebuffers_.Get(defaultFramebufferID).Framebuffer(imageID);
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  constexpr VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Handle());

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

  return FrameContext{.ImageIndex = imageID};
}

std::expected<void, RenderError> Renderer::EndFrame(const FrameContext& ctx) {
  const auto& device = context_.GetDevice();
  auto& sync = context_.GetSync();
  auto& swapchain = context_.GetSwapchain();

  const auto& cmd = context_.GetCommand();
  const auto commandBuffer = cmd.Buffer(currentFrame_);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    return std::unexpected(RenderError::RecordCommandFailed);
  }

  const VkSemaphore waitSemaphores[] = {sync.ImageAvailableSemaphore(currentFrame_)};
  const VkSemaphore signalSemaphores[] = {sync.RenderFinishedSemaphore(ctx.ImageIndex)};
  constexpr VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(device.GraphicsQueue(), 1, &submitInfo, sync.InFlightFence(currentFrame_)) != VK_SUCCESS) {
    return std::unexpected(RenderError::SubmitCommandFailed);
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  const VkSwapchainKHR swapChains[] = {swapchain.Handle()};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &ctx.ImageIndex;
  presentInfo.pResults = nullptr; // Optional

  const VkResult result = vkQueuePresentKHR(device.PresentQueue(), &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || context_.GetFramebufferHasResized()) {
    swapchain.Recreate();
    context_.SetFramebufferHasResized(false);
  } else if (result != VK_SUCCESS) {
    return std::unexpected(RenderError::PresentFailed);
  }
  currentFrame_ = (currentFrame_ + 1) % config::MaxFramesInFlight;

  return {};
}

void Renderer::Submit(const MeshHandle& handle) {
  const auto& cmd = context_.GetCommand();
  const auto commandBuffer = cmd.Buffer(currentFrame_);

  const MeshGPU& gpuMesh = meshes_.Get(handle.MeshID);
  const VkBuffer vertexBuffers[] = {gpuMesh.VertexBuffer.Buffer};
  constexpr VkDeviceSize offsets[] = {0};

  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer, gpuMesh.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT16);
  // &descriptorSets_[currentFrame], 0, nullptr);

  auto& pipeline = pipelineCache_.GetPipeline(PipelineHandle{defaultPipelineID});
  vkCmdBindDescriptorSets(commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.PipelineLayout(),
                          0,
                          1,
                          &descriptorSets_[currentFrame_],
                          0,
                          nullptr);
  vkCmdDrawIndexed(commandBuffer, gpuMesh.IndexCount, 1, 0, 0, 0);
  // vkCmdDraw(commandBuffer, gpuMesh.VertexCount, 1, 0, 0);
}

bool Renderer::ShouldClose() const noexcept {
  return context_.WindowShouldClose();
}

void Renderer::WaitUntilIdle() const {
  context_.WaitUntilIdle();
}

PipelineHandle Renderer::CreatePipeline(const PipelineCreateInfo& info) {
  return pipelineCache_.CreatePipeline(info, renderPasses_.Get(0));
}

void Renderer::DeletePipeline(const PipelineHandle handle) {
  pipelineCache_.DestroyPipeline(handle);
}

MeshHandle Renderer::CreateMesh(const Mesh& mesh) {
  const AllocatedBuffer gpuVertex = createVertexBuffer_(mesh);
  const AllocatedBuffer gpuIndex = createIndexBuffer_(mesh);

  // Construct Mesh, return handle.
  MeshGPU gpuMesh{.VertexCount = static_cast<uint32_t>(mesh.Vertices.size()),
                  .IndexCount = static_cast<uint32_t>(mesh.Indices.size()),
                  .VertexBuffer = gpuVertex,
                  .IndexBuffer = gpuIndex};

  const uint32_t meshID = meshes_.Create(gpuMesh);
  return MeshHandle{.MeshID = meshID};
}

void Renderer::DeleteMesh(const MeshHandle handle) {
  const auto& device = context_.GetDevice();
  device.DestroyBuffer(meshes_.Get(handle.MeshID).VertexBuffer);
  device.DestroyBuffer(meshes_.Get(handle.MeshID).IndexBuffer);
  meshes_.Delete(handle.MeshID);
}

AllocatedBuffer Renderer::createVertexBuffer_(const Mesh& mesh) {
  const auto& device = context_.GetDevice();
  const auto& vkDevice = device.Logical();

  const VkDeviceSize bufferSize = sizeof(mesh.Vertices[0]) * mesh.Vertices.size();
  // Create Staging Buffer.
  const AllocatedBuffer staging =
      device.CreateBuffer(bufferSize,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // Fill Staging Buffer
  void* data{};
  vkMapMemory(vkDevice, staging.Memory, 0, bufferSize, 0, &data);
  std::memcpy(data, mesh.Vertices.data(), bufferSize);
  vkUnmapMemory(vkDevice, staging.Memory);

  // Allocate Vertex Buffer.
  const AllocatedBuffer gpuVertex =
      device.CreateBuffer(bufferSize,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  // Staging Buffer to Vertex Buffer.
  device.CopyBuffer(staging.Buffer, gpuVertex.Buffer, bufferSize);
  device.DestroyBuffer(staging);
  return gpuVertex;
}

AllocatedBuffer Renderer::createIndexBuffer_(const Mesh& mesh) {
  const auto& device = context_.GetDevice();
  const auto& vkDevice = device.Logical();

  const VkDeviceSize bufferSize = sizeof(mesh.Indices[0]) * mesh.Indices.size();
  // Create Staging Buffer.
  const AllocatedBuffer staging =
      device.CreateBuffer(bufferSize,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // Fill Staging Buffer
  void* data{};
  vkMapMemory(vkDevice, staging.Memory, 0, bufferSize, 0, &data);
  std::memcpy(data, mesh.Indices.data(), bufferSize);
  vkUnmapMemory(vkDevice, staging.Memory);

  // Allocate Index Buffer.
  const AllocatedBuffer gpuIndices =
      device.CreateBuffer(bufferSize,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  // Staging Buffer to Index Buffer.
  device.CopyBuffer(staging.Buffer, gpuIndices.Buffer, bufferSize);
  device.DestroyBuffer(staging);
  return gpuIndices;
}

} // namespace engine::graphics::vulkan