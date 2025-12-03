#include "RenderObject.hpp"

#include "PipelineCreateInfo.hpp"
#include "Engine/Graphics/Vulkan/Device.hpp"
#include "Engine/Graphics/Vulkan/Command.hpp"
#include "Engine/Graphics/Vulkan/Context.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Vulkan/Pipeline.hpp"
#include "Vulkan/PipelineCache.hpp"
#include "Vulkan/SwapChain.hpp"

#include <cstring>

namespace engine::graphics {

RenderObject::RenderObject(vulkan::Context& context) : context_(context) {}

RenderObject::~RenderObject() {
  const auto vkDevice = context_.GetDevice().Logical();

  vkDestroyBuffer(vkDevice, vertexBuffer_, nullptr);
  vkFreeMemory(vkDevice, vertexBufferMemory_, nullptr);
  vkDestroyBuffer(vkDevice, indexBuffer_, nullptr);
  vkFreeMemory(vkDevice, indexBufferMemory_, nullptr);
}

void RenderObject::Record(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame) {

  VkBuffer vertexBuffers[] = {vertexBuffer_};
  VkDeviceSize offsets[] = {0};

  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  // vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT16);
  // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.PipelineLayout(), 0, 1,
  // &descriptorSets_[currentFrame], 0, nullptr);

  // vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Indices.size()), 1, 0, 0, 0);
  vkCmdDraw(commandBuffer, verticesSize_, 1, 0, 0);
}

void RenderObject::UploadMesh(Mesh& mesh) {
  createVertexBuffer_(mesh.Vertices);
}

void RenderObject::createVertexBuffer_(const std::vector<Vertex>& vertices) {
  const auto vkDevice = context_.GetDevice().Logical();

  verticesSize_ = vertices.size();
  const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer_(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                stagingBufferMemory);

  void* data;
  vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
  std::memcpy(data, vertices.data(), bufferSize);
  vkUnmapMemory(vkDevice, stagingBufferMemory);

  createBuffer_(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer_, vertexBufferMemory_);
  copyBuffer_(stagingBuffer, vertexBuffer_, bufferSize);

  vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
  vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
}

void RenderObject::createIndexBuffer_(std::vector<uint32_t> indices) {
  const auto vkDevice = context_.GetDevice().Logical();

  const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer_(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                stagingBufferMemory);

  void* data;
  vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices.data(), bufferSize);
  vkUnmapMemory(vkDevice, stagingBufferMemory);

  createBuffer_(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer_, indexBufferMemory_);

  copyBuffer_(stagingBuffer, indexBuffer_, bufferSize);

  vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
  vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
}

void RenderObject::createBuffer_(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                                 VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
  const auto& device = context_.GetDevice();

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
  vkBindBufferMemory(device.Logical(), buffer, bufferMemory, 0);
}

void RenderObject::copyBuffer_(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  auto& cmd = context_.GetCommand();
  VkCommandBuffer commandBuffer = cmd.BeginSingleTimeCommands_();

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0; // Optional
  copyRegion.dstOffset = 0; // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  cmd.EndSingleTimeCommands_(commandBuffer);
}

} // namespace engine::graphics
