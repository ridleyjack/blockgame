#include "WorldGenerator.hpp"

#include "BlockRegistry.hpp"
#include "Map.hpp"

#include <cmath>

namespace {
// Value noise.
constexpr std::uint64_t Hash(std::uint64_t x) noexcept {
  x ^= x >> 30;
  x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27;
  x *= 0x94d049bb133111ebULL;
  x ^= x >> 31;
  return x;
}

constexpr double Lerp(const double a, const double b, const double t) noexcept {
  return a + (b - a) * t;
}

constexpr double Smoothstep(const double t) noexcept {
  return t * t * (3.0 - 2.0 * t);
}

constexpr double Random01(const std::uint64_t seed, const int x, const int z) noexcept {
  const auto h = Hash(seed ^ (static_cast<std::uint64_t>(x) * 0x9e3779b97f4a7c15ULL) ^
                      (static_cast<std::uint64_t>(z) * 0xbf58476d1ce4e5b9ULL));

  return static_cast<double>(h >> 11) * (1.0 / static_cast<double>(1ULL << 53));
}

double ValueNoise2D(const std::uint64_t seed, const double x, const double z) noexcept {
  const int x0 = static_cast<int>(std::floor(x));
  const int z0 = static_cast<int>(std::floor(z));
  const int x1 = x0 + 1;
  const int z1 = z0 + 1;

  const double tx = Smoothstep(x - static_cast<double>(x0));
  const double tz = Smoothstep(z - static_cast<double>(z0));

  const double a = Random01(seed, x0, z0);
  const double b = Random01(seed, x1, z0);
  const double c = Random01(seed, x0, z1);
  const double d = Random01(seed, x1, z1);

  return Lerp(Lerp(a, b, tx), Lerp(c, d, tx), tz);
}

constexpr int GenerateHeight(const std::uint64_t seed, int worldX, int worldZ) {
  const double lowFreq = ValueNoise2D(seed, worldX * 0.015, worldZ * 0.015);
  const double highFreq = ValueNoise2D(seed, worldX * 0.060, worldZ * 0.060);

  const double combined = lowFreq * 0.75 + highFreq * 0.25;

  constexpr int baseHeight = 24;
  constexpr int amplitude = 16;

  return baseHeight + static_cast<int>(combined * amplitude);
}

} // namespace

BlockType WorldGenerator::BlockAt(math::Vec3Int worldCoord) {
  const int height = GenerateHeight(seed, worldCoord.X, worldCoord.Z);

  if (worldCoord.Y > height)
    return BlockType::Air;

  if (worldCoord.Y == height)
    return BlockType::Grass;

  if (worldCoord.Y >= height - 4)
    return BlockType::Dirt;

  return BlockType::Stone;
}

void WorldGenerator::GenerateChunk(Chunk& chunk, const math::Vec3Int chunkCoord) {
  for (int z = 0; z < chunk.Blocks.Depth(); z++)
    for (int y = 0; y < chunk.Blocks.Depth(); y++)
      for (int x = 0; x < chunk.Blocks.Depth(); x++) {
        const math::Vec3Int worldCoord{.X = static_cast<std::int32_t>(chunkCoord.X * ChunkWidth + x),
                                       .Y = static_cast<std::int32_t>(chunkCoord.Y * ChunkHeight + y),
                                       .Z = static_cast<std::int32_t>(chunkCoord.Z * ChunkDepth + z)};

        BlockType block = BlockAt(worldCoord);
        chunk.Blocks[z, y, x] = static_cast<std::uint8_t>(block);
      }
}
