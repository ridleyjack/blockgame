#include "MeshBuffer.hpp"

#include "CheckVk.hpp"
#include "Context.hpp"
#include "Device.hpp"

#include "Engine/Graphics/Mesh.hpp"

namespace engine::graphics::vulkan {

MeshBuffer MeshBuffer::Create(Context& context) {
  const auto& device = context.GetDevice();
  auto vkDevice = device.Logical();

  VkDeviceSize capacity{};
  VkBuffer buffer{VK_NULL_HANDLE};
  VkDeviceMemory memory{VK_NULL_HANDLE};

  constexpr VkBufferCreateInfo bufferInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = DefaultBufferMemorySize,
      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  CheckVk(vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer), "vkCreateBuffer");

  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(vkDevice, buffer, &req);
  capacity = req.size;
  const std::uint32_t memoryType = device.FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkMemoryAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = req.size,
      .memoryTypeIndex = memoryType,
  };
  const VkResult allocateResult = vkAllocateMemory(vkDevice, &allocInfo, nullptr, &memory);
  if (allocateResult != VK_SUCCESS) {
    vkDestroyBuffer(vkDevice, buffer, nullptr);
    CheckVk(allocateResult, "vkAllocateMemory");
  }
  const VkResult bindResult = vkBindBufferMemory(vkDevice, buffer, memory, 0);
  if (bindResult != VK_SUCCESS) {
    vkDestroyBuffer(vkDevice, buffer, nullptr);
    vkFreeMemory(vkDevice, memory, nullptr);
    CheckVk(bindResult, "vkBindBufferMemory");
  }

  return MeshBuffer(context, buffer, memory, capacity);
}

MeshBuffer::~MeshBuffer() {
  auto vkDevice = context_.GetDevice().Logical();
  vkFreeMemory(vkDevice, memory_, nullptr);
  vkDestroyBuffer(vkDevice, buffer_, nullptr);
}

VkBuffer MeshBuffer::Handle() const noexcept {
  return buffer_;
}

std::expected<VkDeviceSize, memory::SparseBuffer::AllocateError> MeshBuffer::Allocate(const VkDeviceSize size) {
  constexpr VkDeviceSize alignment = std::max<VkDeviceSize>(alignof(Vertex), alignof(std::uint32_t));
  const auto offset = sparseBuffer_.Allocate(size, alignment);
  if (!offset)
    return std::unexpected(offset.error());
  return *offset;
}

void MeshBuffer::Free(const VkDeviceSize offset) {
  sparseBuffer_.Free(offset);
}

MeshBuffer::MeshBuffer(Context& context, VkBuffer buffer, VkDeviceMemory memory, const VkDeviceSize capacity)
    : context_(context), buffer_(buffer), memory_(memory), sparseBuffer_(capacity) {}

} // namespace engine::graphics::vulkan
