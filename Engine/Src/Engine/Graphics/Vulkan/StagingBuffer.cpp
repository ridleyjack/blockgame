#include "StagingBuffer.hpp"

#include "Context.hpp"
#include "Device.hpp"

namespace engine::graphics::vulkan {

StagingBuffer::StagingBuffer(Context& context) : context_(context) {
  const auto& device = context.GetDevice();
  auto vkDevice = device.Logical();

  constexpr VkBufferCreateInfo bufferInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = DefaultBufferMemorySize,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer_) != VK_SUCCESS)
    throw std::runtime_error("failed to create dummy vertex/index buffer!");

  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(vkDevice, buffer_, &req);
  capacity_ = req.size;
  const std::uint32_t memoryType =
      device.FindMemoryType(req.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = req.size;
  allocInfo.memoryTypeIndex = memoryType;
  if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &memory_) != VK_SUCCESS) {
    vkDestroyBuffer(vkDevice, buffer_, nullptr);
    throw std::runtime_error("failed to allocate vertex buffer memory!");
  }
  vkBindBufferMemory(vkDevice, buffer_, memory_, 0);

  if (vkMapMemory(vkDevice, memory_, 0, capacity_, 0, &mapped_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to map staging buffer memory");
  }
}

StagingBuffer::~StagingBuffer() {
  auto vkDevice = context_.GetDevice().Logical();
  vkUnmapMemory(vkDevice, memory_);
  vkFreeMemory(vkDevice, memory_, nullptr);
  vkDestroyBuffer(vkDevice, buffer_, nullptr);
}

void* StagingBuffer::Mapping() const noexcept {
  return mapped_;
}

VkDeviceMemory StagingBuffer::Memory() const noexcept {
  return memory_;
}

VkBuffer StagingBuffer::Handle() const noexcept {
  return buffer_;
}

VkDeviceSize StagingBuffer::Allocate(VkDeviceSize size, VkDeviceSize alignment) {
  const VkDeviceSize alignedOffset = align_(offset_, alignment);
  if (alignedOffset + size > capacity_) {
    throw std::runtime_error("buffer size exceeds capacity of allocated memory!");
  }

  offset_ = alignedOffset + size;
  return alignedOffset;
}

constexpr VkDeviceSize StagingBuffer::align_(const VkDeviceSize value, const VkDeviceSize alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace engine::graphics::vulkan