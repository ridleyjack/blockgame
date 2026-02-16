#pragma once
#include <cstdint>

namespace engine::math {
struct Vec3Int {
  std::int32_t X{};
  std::int32_t Y{};
  std::int32_t Z{};

  constexpr Vec3Int operator+(Vec3Int rhs) const noexcept {
    return {X + rhs.X, Y + rhs.Y, Z + rhs.Z};
  }

  constexpr Vec3Int operator-(Vec3Int rhs) const noexcept {
    return {X - rhs.X, Y - rhs.Y, Z - rhs.Z};
  }

  constexpr bool operator==(const Vec3Int&) const noexcept = default;
};
} // namespace engine::math
