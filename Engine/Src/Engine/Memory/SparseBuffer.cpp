#include "SparseBuffer.hpp"

#include "Alignment.hpp"

#include <cassert>
#include <ios>
#include <limits>
#include <bits/fs_fwd.h>

namespace engine::memory {

SparseBuffer::SparseBuffer(const std::uint64_t size) : capacity_(size) {
  freeBlocks_.push_back({.Offset = 0, .Size = size});
}

std::uint64_t SparseBuffer::Allocate(const std::uint64_t size, const std::uint64_t alignment) {
  // Alignment must be a power of 2 or 1.
  assert(std::has_single_bit(alignment));

  for (size_t i = 0; i < freeBlocks_.size(); i++) {
    const auto& freeBlock = freeBlocks_[i];

    const std::uint64_t alignedOffset{memory::Align(freeBlock.Offset, alignment)};
    const std::uint64_t padding{alignedOffset - freeBlock.Offset};

    if (padding > freeBlock.Size)
      continue; // Underflow.
    if (size > freeBlock.Size - padding)
      continue; // Not enough space.

    const std::uint64_t consumed{size + padding};

    // Record of the total allocated space.
    const BufferBlock allocated{.Offset = freeBlock.Offset, .Size = consumed};
    allocatedBlocks_[alignedOffset] = allocated;

    // New empty space record.
    BufferBlock emptyBlock{.Offset = freeBlock.Offset + consumed, .Size = freeBlock.Size - consumed};
    if (emptyBlock.Size > 0)
      freeBlocks_[i] = emptyBlock;
    else
      freeBlocks_.erase(freeBlocks_.begin() + i);
    return alignedOffset;
  }
  return std::numeric_limits<uint64_t>::max();
}

void SparseBuffer::Free(const std::uint64_t alignedOffset) {
  const auto it = allocatedBlocks_.find(alignedOffset);
  if (it == allocatedBlocks_.end())
    return;

  std::uint64_t offset{it->second.Offset};
  std::uint64_t size{it->second.Size};
  allocatedBlocks_.erase(it);

  if (freeBlocks_.empty()) {
    freeBlocks_.push_back({offset, size});
    return;
  }

  size_t idx = 0;
  while (idx < freeBlocks_.size() && freeBlocks_[idx].Offset < offset) {
    idx++;
  }

  while (idx > 0) {
    auto& left = freeBlocks_[idx - 1];
    if (left.Offset + left.Size != offset)
      break;

    offset = left.Offset;
    size += left.Size;
    freeBlocks_.erase(freeBlocks_.begin() + idx - 1);
    idx--;
  }

  while (idx < freeBlocks_.size()) {
    auto& right = freeBlocks_[idx];
    if (offset + size != right.Offset)
      break;
    size += right.Size;
    freeBlocks_.erase(freeBlocks_.begin() + idx);
  }

  freeBlocks_.insert(freeBlocks_.begin() + idx, {.Offset = offset, .Size = size});
}

std::uint64_t SparseBuffer::Capacity() const noexcept {
  return capacity_;
}

std::uint64_t SparseBuffer::FreeCapacity() const noexcept {
  std::uint64_t free{0};
  for (size_t i = 0; i < freeBlocks_.size(); i++) {
    free += freeBlocks_[i].Size;
  }
  return free;
}

} // namespace engine::memory