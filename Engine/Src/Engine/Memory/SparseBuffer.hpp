#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <expected>

namespace engine::memory {

// SparseBuffer is a first-fit free-list allocator for a fixed-capacity contiguous byte region.
// Free adjacent blocks are merged to reduce fragmentation.
class SparseBuffer {
public:
  enum class AllocateError : std::uint8_t {
    OutOfCapacity,
    SizeOverflow,
  };

  explicit SparseBuffer(std::uint64_t size);

  std::expected<std::uint64_t, AllocateError> Allocate(std::uint64_t size, std::uint64_t alignment);
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