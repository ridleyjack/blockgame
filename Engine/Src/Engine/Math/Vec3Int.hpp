#pragma once
#include <cstdint>
#include <functional>

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

struct Vec3IntHash {
  std::size_t operator()(const engine::math::Vec3Int& v) const noexcept {
    std::size_t seed = std::hash<std::int32_t>{}(v.X);
    seed ^= std::hash<std::int32_t>{}(v.Y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<std::int32_t>{}(v.Z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};
} // namespace engine::math
