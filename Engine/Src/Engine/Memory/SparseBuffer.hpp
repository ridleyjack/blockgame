#pragma once

#include <cstdint>
#include <vector>
#include <map>

namespace engine::memory {

// SparseBuffer is a first-fit free-list allocator for a fixed-capacity contiguous byte region.
// Returns std::numeric_limits<uint64_t>::max() on failure. (Not enough space or overflow)
// Free adjacent blocks are merged to reduce fragmentation.
class SparseBuffer {
public:
  SparseBuffer(std::uint64_t size);

  std::uint64_t Allocate(std::uint64_t size, std::uint64_t alignment);
  void Free(std::uint64_t alignedOffset);

  std::uint64_t Capacity() const noexcept;
  std::uint64_t FreeCapacity() const noexcept;

private:
  struct BufferBlock {
    std::uint64_t Offset{};
    std::uint64_t Size{};
  };

  std::uint64_t capacity_{};
  std::vector<BufferBlock> freeBlocks_{};
  std::map<std::uint64_t, BufferBlock> allocatedBlocks_{};
};

} // namespace engine::memory