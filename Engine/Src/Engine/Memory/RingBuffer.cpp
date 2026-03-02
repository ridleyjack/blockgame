#include "RingBuffer.hpp"

#include "Alignment.hpp"

#include <limits>

namespace engine::memory {

RingBuffer::RingBuffer(std::uint64_t size) : capacity_(size) {}

std::uint64_t RingBuffer::Allocate(std::uint64_t size, std::uint64_t alignment) {
  if (head_ == tail_ && !allocatedBlocks_.empty())
    return std::numeric_limits<std::uint64_t>::max();

  std::uint64_t aligned = Align(head_, alignment);

  if (head_ >= tail_) { // Head is chasing end of buffer.
    if (aligned > capacity_)
      return std::numeric_limits<std::uint64_t>::max();
    if (size > capacity_ - aligned) {
      if (size <= tail_) { // Try to wrap.
        head_ = 0;
        aligned = 0;
      } else
        return std::numeric_limits<std::uint64_t>::max();
    }
  } else { // Head is chasing tail.
    if (aligned > tail_) {
      return std::numeric_limits<std::uint64_t>::max();
    }
    if (size > tail_ - aligned)
      return std::numeric_limits<std::uint64_t>::max();
  }

  allocatedBlocks_.emplace_back(head_, aligned, aligned + size - head_);
  head_ = aligned + size;

  return aligned;
}

std::uint64_t RingBuffer::PopFront() {
  if (allocatedBlocks_.empty())
    return std::numeric_limits<std::uint64_t>::max();

  BufferBlock& block = allocatedBlocks_.front();
  std::uint64_t alignedOffset{block.AlignedOffset};

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

std::uint64_t RingBuffer::PeekFront() {
  if (allocatedBlocks_.empty())
    return std::numeric_limits<std::uint64_t>::max();

  return allocatedBlocks_.begin()->AlignedOffset;
}

std::uint64_t RingBuffer::Capacity() const noexcept {
  return capacity_;
}

} // namespace engine::memory