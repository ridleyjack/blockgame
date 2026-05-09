#include "ChunkStreamer.hpp"

#include "ChunkMesher.hpp"

#include <ranges>

ChunkStreamer::ChunkStreamer(WorldGenerator& generator, ChunkMesher& mesher)
    : worldGenerator_(generator), mesher_(mesher) {
  const std::size_t size = (2 * LoadRadius + 1) * (2 * LoadRadius + 1) * worldGenerator_.WorldHeight;
  loadedChunks_.resize(size);
}

void ChunkStreamer::Update(const math::Vec3Int playerChunk) {
  std::vector<std::optional<math::Vec3Int>> needed{};
  needed.reserve(loadedChunks_.size());

  // Find needed chunks.
  constexpr int radius = LoadRadius;
  for (int dZ = -radius; dZ <= radius; dZ++)
    for (int dX = -radius; dX <= radius; dX++) {
      const int chunkZ = playerChunk.Z + dZ;
      const int chunkX = playerChunk.X + dX;
      if (chunkZ < 0 || chunkZ >= worldGenerator_.WorldDepth || chunkX < 0 || chunkX >= worldGenerator_.WorldWidth)
        continue;

      for (std::int32_t y = 0; y < worldGenerator_.WorldHeight; y++) {
        needed.push_back({
            {chunkX, y, chunkZ}
        });
      }
    }

  // Remove unneeded, chunks.
  for (auto& loaded : loadedChunks_) {
    if (!loaded)
      continue;

    auto found = std::ranges::find(needed, *loaded);
    if (found != needed.end()) {
      *found = std::nullopt;
      continue;
    }
    mesher_.RequestUnload(*loaded);
    loaded = std::nullopt;
  }

  // Add needed chunks
  for (auto& chunk : needed) {
    if (!chunk)
      continue;

    mesher_.RequestLoad(*chunk);
    for (auto& loaded : loadedChunks_) {
      if (loaded)
        continue; // Look for next empty spot.

      loaded = chunk;
      break;
    }
  }
}

std::span<const std::optional<math::Vec3Int>> ChunkStreamer::LoadedChunks() const noexcept {
  return loadedChunks_;
}
