#pragma once

#include <cassert>
#include <cstdint>
#include <optional>
#include <bit>

namespace engine::memory {

// Align returns the smallest value >= value that is a multiple of alignment.
// Alignment must be 1 or a power of two.
// No value is returned on overflow.
constexpr std::optional<std::uint64_t> Align(const std::uint64_t value, const std::uint64_t alignment) noexcept {
  assert(std::has_single_bit(alignment));
  const std::uint64_t mask = alignment - 1;

  std::uint64_t sum{};
  if (__builtin_add_overflow(value, mask, &sum))
    return std::nullopt;
  return sum & ~mask;
}

} // namespace engine::memory
