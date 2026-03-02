#pragma once

#include <cstdint>
#include <deque>

namespace engine::memory {

class RingBuffer {
public:
  explicit RingBuffer(std::uint64_t size);

  std::uint64_t Allocate(std::uint64_t size, std::uint64_t alignment);
  std::uint64_t PopFront();
  std::uint64_t PeekFront();

  std::uint64_t Capacity() const noexcept;

private:
  struct BufferBlock {
    std::uint64_t Offset{};
    std::uint64_t AlignedOffset{};
    std::uint64_t Size{};
  };

  std::uint64_t capacity_{};
  std::deque<BufferBlock> allocatedBlocks_{};

  std::uint64_t head_{};
  std::uint64_t tail_{};
};

} // namespace engine::memory