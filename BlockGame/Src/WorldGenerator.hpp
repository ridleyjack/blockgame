#pragma once

#include "Grid3D.hpp"

#include <cstdint>

#include "Engine/Math/Vec3Int.hpp"

enum class BlockType : std::uint8_t;
namespace math = engine::math;

constexpr std::uint32_t ChunkWidth{16};
constexpr std::uint32_t ChunkHeight{16};
constexpr std::uint32_t ChunkDepth{16};

struct Chunk {
  Grid3D<std::uint8_t> Blocks{ChunkDepth, ChunkHeight, ChunkWidth, 0};
};

class WorldGenerator {
public:
  static constexpr std::uint32_t WorldWidth{100};
  static constexpr std::uint32_t WorldHeight{3};
  static constexpr std::uint32_t WorldDepth{100};

  BlockType BlockAt(math::Vec3Int worldCoord);
  Chunk GenerateChunk(math::Vec3Int chunkCoord);

private:
  std::uint64_t seed{};
};
