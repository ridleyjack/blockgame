#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <bit>

namespace engine::memory {

// Align returns the smallest value >= value that is a multipl of alignment.
// Alignment must be 1 or a power of two.
// std::numeric_limits<std::uint64_t>::max() is returned on overflow.
constexpr std::uint64_t Align(const std::uint64_t value, const std::uint64_t alignment) noexcept {
  assert(std::has_single_bit(alignment));
  const std::uint64_t mask = alignment - 1;

  std::uint64_t sum{};
  if (__builtin_add_overflow(value, mask, &sum))
    return std::numeric_limits<std::uint64_t>::max();

  return sum & ~mask;
}

} // namespace engine::memory
