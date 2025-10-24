#include "TileGroup.hpp"

#include "Vulkan/VulkanPipelineCache.hpp"
#include "Vulkan/VulkanCommand.hpp"
#include "Vulkan/VulkanContext.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "Vulkan/VulkanPipeline.hpp"
#include "Vulkan/VulkanSwapChain.hpp"

#include "Engine/Assets//ImageLoader.hpp"

#include <chrono>
#include <cstring>

namespace Engine {

// ==============================
// Public Methods
// ==============================

TileGroup::TileGroup(Context& context) : context_{context} {}

TileGroup::~TileGroup() {
  auto vkDevice = context_.GetDevice().Logical();

  vkDestroySampler(vkDevice, textureSampler_, nullptr);
  vkDestroyImageView(vkDevice, textureImageView_, nullptr);
  vkDestroyImage(vkDevice, textureImage_, nullptr);
  vkFreeMemory(vkDevice, textureImageMemory_, nullptr);

  vkDestroyDescriptorPool(vkDevice, descriptorPool_, nullptr);
}

void TileGroup::Init() {
  CreatePipelineRequest request{"Shaders/vert.spv", "Shaders/frag.spv"};
  context_.GetPipelineLibrary().CreatePipeline(request);

  createTextureImage_();
  createTextureImageView_();
  createTextureSampler_();
  createVertexBuffer_();
  createIndexBuffer_();
  createUniformBuffers_();
  createDescriptorPool_();
  createDescriptorSets_();
}

void TileGroup::UpdateUniformBuffer(uint32_t currentImage) {
  auto& swapchain = context_.GetSwapchain();

  static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  UniformBufferObject ubo{};
  ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.Proj = glm::perspective(
      glm::radians(45.0f), static_cast<float>(swapchain.Extent().width) / static_cast<float>(swapchain.Extent().height),
      0.1f, 10.0f);
  ubo.Proj[1][1] *= -1;

  memcpy(uniformBuffersMapped_[currentImage], &ubo, sizeof(ubo));
}

void TileGroup::Record(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;                  // Optional
  beginInfo.pInheritanceInfo = nullptr; // Optional

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  VkExtent2D swapChainExtent = context_.GetSwapchain().Extent();
  Pipeline& pipeline = context_.GetPipelineLibrary().GetPipeline(0);

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = pipeline.RenderPass();
  renderPassInfo.framebuffer = context_.GetSwapchain().Framebuffer(imageIndex);
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

  VkBuffer vertexBuffers[] = {vertexBuffer_};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT16);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.PipelineLayout(), 0, 1,
                          &descriptorSets_[currentFrame], 0, nullptr);
  vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Indices.size()), 1, 0, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

// ==============================
// Private Methods
// ==============================

void TileGroup::createTextureImage_() {
  auto& device = context_.GetDevice();

  Assets::ImageLoader texture{};
  texture.Load("Textures/texture.jpg");
  VkDeviceSize imageSize = texture.Width() * texture.Height() * 4;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer_(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                stagingBufferMemory);

  std::fprintf(stderr, "[map] dev=%p mem=%p size=%zu\n", (void*)device.Logical(), (void*)stagingBufferMemory,
               (size_t)imageSize);
  void* data;
  vkMapMemory(device.Logical(), stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, texture.Data(), static_cast<size_t>(imageSize));
  vkUnmapMemory(device.Logical(), stagingBufferMemory);

  createImage_(texture.Width(), texture.Height(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               textureImage_, textureImageMemory_);

  transitionImageLayout_(textureImage_, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage_(stagingBuffer, textureImage_, static_cast<uint32_t>(texture.Width()),
                     static_cast<uint32_t>(texture.Height()));
  transitionImageLayout_(textureImage_, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(device.Logical(), stagingBuffer, nullptr);
  vkFreeMemory(device.Logical(), stagingBufferMemory, nullptr);
}

void TileGroup::createTextureImageView_() {
  auto& device = context_.GetDevice();

  textureImageView_ = device.CreateImageView(textureImage_, VK_FORMAT_R8G8B8A8_SRGB);
}

void TileGroup::createTextureSampler_() {
  auto& device = context_.GetDevice();

  // TODO: Might better to make this available to all functions.
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device.Physical(), &properties);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  if (vkCreateSampler(device.Logical(), &samplerInfo, nullptr, &textureSampler_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

void TileGroup::createVertexBuffer_() {
  auto vkDevice = context_.GetDevice().Logical();

  VkDeviceSize bufferSize = sizeof(Vertices[0]) * Vertices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer_(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                stagingBufferMemory);

  void* data;
  vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
  std::memcpy(data, Vertices.data(), bufferSize);
  vkUnmapMemory(vkDevice, stagingBufferMemory);

  createBuffer_(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer_, vertexBufferMemory_);
  copyBuffer_(stagingBuffer, vertexBuffer_, bufferSize);

  vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
  vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
}

void TileGroup::createIndexBuffer_() {
  auto vkDevice = context_.GetDevice().Logical();

  VkDeviceSize bufferSize = sizeof(Indices[0]) * Indices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer_(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                stagingBufferMemory);

  void* data;
  vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, Indices.data(), bufferSize);
  vkUnmapMemory(vkDevice, stagingBufferMemory);

  createBuffer_(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer_, indexBufferMemory_);

  copyBuffer_(stagingBuffer, indexBuffer_, bufferSize);

  vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
  vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
}

void TileGroup::createUniformBuffers_() {
  auto vkDevice = context_.GetDevice().Logical();

  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  uniformBuffers_.resize(Engine::config::MaxFramesInFlight);
  uniformBuffersMemory_.resize(Engine::config::MaxFramesInFlight);
  uniformBuffersMapped_.resize(Engine::config::MaxFramesInFlight);

  for (size_t i = 0; i < Engine::config::MaxFramesInFlight; i++) {
    createBuffer_(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers_[i],
                  uniformBuffersMemory_[i]);

    vkMapMemory(vkDevice, uniformBuffersMemory_[i], 0, bufferSize, 0, &uniformBuffersMapped_[i]);
  }
}

void TileGroup::createDescriptorPool_() {
  auto vkDevice = context_.GetDevice().Logical();

  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(Engine::config::MaxFramesInFlight);
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = static_cast<uint32_t>(Engine::config::MaxFramesInFlight);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(Engine::config::MaxFramesInFlight);

  if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void TileGroup::createDescriptorSets_() {
  auto vkDevice = context_.GetDevice().Logical();
  auto& pipeline = context_.GetPipelineLibrary().GetPipeline(0);

  std::vector<VkDescriptorSetLayout> layouts(Engine::config::MaxFramesInFlight, pipeline.DescriptorSetLayout());
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool_;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(Engine::config::MaxFramesInFlight);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets_.resize(Engine::config::MaxFramesInFlight);
  if (vkAllocateDescriptorSets(vkDevice, &allocInfo, descriptorSets_.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < Engine::config::MaxFramesInFlight; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers_[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageView_;
    imageInfo.sampler = textureSampler_;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSets_[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSets_[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(vkDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
  }
}

void TileGroup::createBuffer_(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                              VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
  auto& device = context_.GetDevice();

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device.Logical(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create vertex buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device.Logical(), buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = device.FindMemoryType(memRequirements.memoryTypeBits, properties);
  if (vkAllocateMemory(device.Logical(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate vertex buffer memory!");
  }
  std::fprintf(stderr, "alloc mem=%p", (void*)bufferMemory);

  vkBindBufferMemory(device.Logical(), buffer, bufferMemory, 0);
}

void TileGroup::copyBuffer_(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  auto& cmd = context_.GetCommand();
  VkCommandBuffer commandBuffer = cmd.BeginSingleTimeCommands_();

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0; // Optional
  copyRegion.dstOffset = 0; // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  cmd.EndSingleTimeCommands_(commandBuffer);
}

void TileGroup::createImage_(int32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                             VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
                             VkDeviceMemory& imageMemory) {
  auto& device = context_.GetDevice();

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = static_cast<uint32_t>(width);
  imageInfo.extent.height = static_cast<uint32_t>(height);
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = 0; // Optional

  if (vkCreateImage(device.Logical(), &imageInfo, nullptr, &textureImage_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device.Logical(), textureImage_, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      device.FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device.Logical(), &allocInfo, nullptr, &textureImageMemory_) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vkBindImageMemory(device.Logical(), textureImage_, textureImageMemory_, 0);
}

void TileGroup::transitionImageLayout_(VkImage image, VkFormat format, VkImageLayout oldLayout,
                                       VkImageLayout newLayout) {
  auto& device = context_.GetDevice();
  auto& cmd = context_.GetCommand();

  VkCommandBuffer commandBuffer = cmd.BeginSingleTimeCommands_();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;
  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

  cmd.EndSingleTimeCommands_(commandBuffer);
}

void TileGroup::copyBufferToImage_(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
  auto& cmd = context_.GetCommand();

  VkCommandBuffer commandBuffer = cmd.BeginSingleTimeCommands_();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};
  vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  cmd.EndSingleTimeCommands_(commandBuffer);
}

} // namespace Engine