#pragma once

#include "Engine/Memory/RingBuffer.hpp"

#include <vulkan/vulkan.h>

#include <deque>
#include <span>
#include <vector>

namespace engine::graphics::vulkan {
class Context;

class StagingBuffer {
public:
  static constexpr VkDeviceSize DefaultBufferMemorySize = 256ull * 1024ull * 1024ull; // 256 MiB

  static StagingBuffer Create(Context& context);
  ~StagingBuffer();

  StagingBuffer(const StagingBuffer&) = delete;
  StagingBuffer& operator=(const StagingBuffer&) = delete;

  std::uint64_t BeginBatch();
  VkDeviceSize Write(std::span<const std::byte> bytes, VkDeviceSize alignment, std::uint64_t batchID);
  void Free(std::uint64_t batchID);

  VkBuffer Handle() const noexcept;

private:
  struct Batch {
    bool Complete{};
    std::vector<std::uint64_t> Offsets{};
  };

  Context& context_;

  VkBuffer buffer_{VK_NULL_HANDLE};
  VkDeviceMemory memory_{VK_NULL_HANDLE};
  std::byte* mappedMemory_{};

  memory::RingBuffer ringBuffer_;

  std::uint64_t headBatchID{};
  std::uint64_t nextBatchID{};
  std::deque<Batch> batches_{};

  StagingBuffer(Context& context,
                VkBuffer buffer,
                VkDeviceMemory memory,
                std::byte* memoryMapping,
                VkDeviceSize capacity);
};

} // namespace engine::graphics::vulkan