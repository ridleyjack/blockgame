#pragma once

#include "Chunk.hpp"
#include "Engine/Math/Vec3Int.hpp"

#include <cstdint>

enum class BlockType : std::uint8_t;
namespace math = engine::math;

class WorldGenerator {
public:
  BlockType BlockAt(math::Vec3Int worldCoord);
  Chunk GenerateChunk(math::Vec3Int chunkCoord);

private:
  std::uint64_t seed{};
};
