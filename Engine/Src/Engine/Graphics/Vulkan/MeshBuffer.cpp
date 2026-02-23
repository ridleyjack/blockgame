#include "MeshBuffer.hpp"

#include "Context.hpp"
#include "Device.hpp"
#include "Engine/Graphics/Mesh.hpp"

#include <print>

namespace engine::graphics::vulkan {

MeshBuffer::MeshBuffer(Context& context) : context_(context) {
  const auto& device = context.GetDevice();
  auto vkDevice = device.Logical();

  const VkBufferCreateInfo bufferInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = DefaultBufferMemorySize,
      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer_) != VK_SUCCESS)
    throw std::runtime_error("failed to create dummy vertex/index buffer!");

  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(vkDevice, buffer_, &req);
  capacity_ = req.size;
  const std::uint32_t memoryType = device.FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = req.size;
  allocInfo.memoryTypeIndex = memoryType;
  if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &memory_) != VK_SUCCESS) {
    vkDestroyBuffer(vkDevice, buffer_, nullptr);
    throw std::runtime_error("failed to allocate vertex buffer memory!");
  }
  vkBindBufferMemory(vkDevice, buffer_, memory_, 0);
}

MeshBuffer::~MeshBuffer() {
  auto vkDevice = context_.GetDevice().Logical();
  vkFreeMemory(vkDevice, memory_, nullptr);
  vkDestroyBuffer(vkDevice, buffer_, nullptr);
}

VkBuffer MeshBuffer::Handle() const noexcept {
  return buffer_;
}

VkDeviceSize MeshBuffer::AllocateDeviceLocal(const VkDeviceSize size) {
  constexpr VkDeviceSize alignment = std::max<VkDeviceSize>(alignof(Vertex), alignof(std::uint32_t));

  VkDeviceSize alignedOffset = align_(offset_, alignment);
  if (alignedOffset + size > capacity_) {
    throw std::runtime_error("buffer size exceeds capacity of allocated memory!");
  }

  offset_ = alignedOffset + size;

  return alignedOffset;
}

// align_ returns the next offset that satisfies the required alignment.
// The left hand side guarantees that we are inside the next alignment bucket.
// The right hand side rides this down to a multiple of alignment.
// This method only works for powers of 2
constexpr VkDeviceSize MeshBuffer::align_(const VkDeviceSize value, const VkDeviceSize alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace engine::graphics::vulkan