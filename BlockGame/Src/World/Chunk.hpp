#pragma once

#include "BlockTypes.hpp"
#include "Containers/Grid3D.hpp"
#include "Engine/Math/Vec3Int.hpp"

namespace math = engine::math;

struct Chunk {
  static constexpr std::uint32_t ChunkWidth{16};
  static constexpr std::uint32_t ChunkHeight{16};
  static constexpr std::uint32_t ChunkDepth{16};

  Grid3D<std::uint8_t> Blocks{ChunkDepth, ChunkHeight, ChunkWidth, 0};

  BlockType BlockAt(const math::Vec3Int localPosition) const noexcept {
    return static_cast<BlockType>(Blocks[localPosition.Z, localPosition.Y, localPosition.X]);
  }

  void SetBlock(const math::Vec3Int localPosition, BlockType blockType) noexcept {
    Blocks[localPosition.Z, localPosition.Y, localPosition.X] = static_cast<std::uint8_t>(blockType);
  }
};
