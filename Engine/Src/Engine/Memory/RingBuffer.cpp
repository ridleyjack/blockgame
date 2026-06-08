#include "RingBuffer.hpp"

#include "Alignment.hpp"

#include <limits>

namespace engine::memory {

RingBuffer::RingBuffer(const std::uint64_t size) : capacity_(size) {}

std::expected<std::uint64_t, RingBuffer::AllocateError> RingBuffer::Allocate(const std::uint64_t size,
                                                                             const std::uint64_t alignment) {
  if (head_ == tail_ && !allocatedBlocks_.empty())
    return std::unexpected(AllocateError::OutOfCapacity);

  auto aligned = Align(head_, alignment);
  if (!aligned)
    return std::unexpected(AllocateError::SizeOverflow);

  if (head_ >= tail_) { // Head is chasing end of buffer.
    if (*aligned > capacity_)
      return std::unexpected(AllocateError::OutOfCapacity);
    if (size > capacity_ - *aligned) {
      if (size <= tail_) { // Try to wrap.
        head_ = 0;
        *aligned = 0;
      } else
        return std::unexpected(AllocateError::OutOfCapacity);
    }
  } else { // Head is chasing tail.
    if (*aligned > tail_) {
      return std::unexpected(AllocateError::OutOfCapacity);
    }
    if (size > tail_ - *aligned)
      return std::unexpected(AllocateError::OutOfCapacity);
  }

  allocatedBlocks_.emplace_back(head_, *aligned, *aligned + size - head_);
  head_ = *aligned + size;

  return *aligned;
}

std::uint64_t RingBuffer::PopFront() noexcept {
  assert(!Empty());

  BufferBlock& block = allocatedBlocks_.front();
  const std::uint64_t alignedOffset{block.AlignedOffset};

  tail_ = block.Offset + block.Size;
  if (tail_ == capacity_)
    tail_ = 0;

  allocatedBlocks_.pop_front();
  if (allocatedBlocks_.empty()) {
    head_ = 0;
    tail_ = 0;
  }
  return alignedOffset;
}

bool RingBuffer::Empty() const noexcept {
  return allocatedBlocks_.empty();
}

std::uint64_t RingBuffer::PeekFront() const noexcept {
  assert(!Empty());
  
  return allocatedBlocks_.begin()->AlignedOffset;
}

std::uint64_t RingBuffer::Capacity() const noexcept {
  return capacity_;
}

} // namespace engine::memory