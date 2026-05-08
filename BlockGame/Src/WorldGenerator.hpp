#pragma once

#include <cstdint>

#include "Engine/Math/Vec3Int.hpp"

enum class BlockType : std::uint8_t;
namespace math = engine::math;

struct Chunk;

class WorldGenerator {
public:
  BlockType BlockAt(math::Vec3Int worldCoord);
  void GenerateChunk(Chunk& chunk, math::Vec3Int chunkCoord);

private:
  std::uint64_t seed{};
};
