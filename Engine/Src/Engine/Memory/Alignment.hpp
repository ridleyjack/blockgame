#pragma once
#include <cstdint>

namespace engine::memory {

constexpr std::uint64_t Align(const std::uint64_t value, const std::uint64_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace engine::memory
