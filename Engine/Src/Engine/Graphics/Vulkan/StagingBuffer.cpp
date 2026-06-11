#include "StagingBuffer.hpp"

#include "CheckVk.hpp"
#include "Context.hpp"
#include "Device.hpp"

#include <cassert>
#include <cstring>
#include <ranges>

namespace engine::graphics::vulkan {

StagingBuffer StagingBuffer::Create(Context& context) {
  const auto& device = context.GetDevice();
  auto vkDevice = device.Logical();

  constexpr VkBufferCreateInfo bufferInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = DefaultBufferMemorySize,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  VkBuffer buffer{VK_NULL_HANDLE};
  VkDeviceMemory memory{VK_NULL_HANDLE};

  CheckVk(vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer), "vkCreateBuffer");

  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(vkDevice, buffer, &req);

  const std::uint32_t memoryType =
      device.FindMemoryType(req.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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

  void* memoryMapping{};
  const VkResult mapResult = vkMapMemory(vkDevice, memory, 0, req.size, 0, &memoryMapping);
  if (mapResult != VK_SUCCESS) {
    vkDestroyBuffer(vkDevice, buffer, nullptr);
    vkFreeMemory(vkDevice, memory, nullptr);
    CheckVk(mapResult, "vkMapMemory");
  }

  return StagingBuffer(context, buffer, memory, static_cast<std::byte*>(memoryMapping), req.size);
}

StagingBuffer::~StagingBuffer() {
  auto vkDevice = context_.GetDevice().Logical();
  vkUnmapMemory(vkDevice, memory_);
  vkFreeMemory(vkDevice, memory_, nullptr);
  vkDestroyBuffer(vkDevice, buffer_, nullptr);
}

std::uint64_t StagingBuffer::BeginBatch() {
  batches_.emplace_back();
  return nextBatchID++;
}

std::expected<VkDeviceSize, memory::RingBuffer::AllocateError>
StagingBuffer::Write(std::span<const std::byte> bytes, const VkDeviceSize alignment, const std::uint64_t batchID) {
  const std::size_t size{bytes.size()};
  const auto offset = ringBuffer_.Allocate(size, alignment);
  if (!offset) {
    return std::unexpected(offset.error());
  }

  std::memcpy(mappedMemory_ + *offset, bytes.data(), size);

  const std::uint64_t idx = batchID - headBatchID;
  assert(idx < batches_.size());

  auto& batch = batches_[idx];
  batch.Offsets.emplace_back(*offset);
  return *offset;
}

void StagingBuffer::Free(const std::uint64_t batchID) {
  const std::uint64_t idx = batchID - headBatchID;
  if (idx >= batches_.size())
    return;

  batches_[idx].Complete = true;

  while (!batches_.empty()) {
    auto& batch = batches_.front();
    if (!batch.Complete)
      break;

    // Offsets are freed in allocation order.
    for (auto offset : batch.Offsets) {
      assert(ringBuffer_.PeekFront() == offset);
      ringBuffer_.PopFront();
    }
    batches_.pop_front();
    headBatchID++;
  }
}

VkBuffer StagingBuffer::Handle() const noexcept {
  return buffer_;
}

StagingBuffer::StagingBuffer(Context& context,
                             VkBuffer buffer,
                             VkDeviceMemory memory,
                             std::byte* memoryMapping,
                             VkDeviceSize capacity)
    : context_(context), buffer_(buffer), memory_(memory), mappedMemory_(memoryMapping), ringBuffer_(capacity) {}
} // namespace engine::graphics::vulkan
