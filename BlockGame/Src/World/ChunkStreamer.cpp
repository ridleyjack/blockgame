#include "ChunkStreamer.hpp"

#include "ChunkMesher.hpp"
#include "WorldGenerator.hpp"

ChunkStreamer::ChunkStreamer(WorldStore& worldStore, WorldGenerator& generator, ChunkMesher& mesher)
    : worldStore_(worldStore), worldGenerator_(generator), mesher_(mesher) {
  const std::size_t size = (2 * LoadRadius + 1) * (2 * LoadRadius + 1) * worldStore_.WorldHeight;
  loadedChunkList_.reserve(size);
  loadedChunks_.reserve(size);

  const std::size_t dataSize = (2 * (LoadRadius + 1) + 1) * (2 * (LoadRadius + 1) + 1) * worldStore_.WorldHeight;
  loadedDataChunks_.reserve(dataSize);
}

void ChunkStreamer::Update(const math::Vec3Int playerChunk) {
  if (playerChunk == lastPlayerChunk_)
    return;
  lastPlayerChunk_ = playerChunk;

  ChunkSet desiredMeshChunks = buildChunkSet_(playerChunk, LoadRadius);
  ChunkSet desiredDataChunks = buildChunkSet_(playerChunk, LoadRadius + 1);

  auto worldView = worldStore_.AcquireWriteView();

  // Update chunk block data.
  // Chunk mesh generation depends on the block data for the desired chunk to mesh and its neighbors.
  for (auto& chunkCoord : desiredDataChunks) {
    if (const auto result = worldView.GetChunk(chunkCoord); !result) {
      worldView.StoreChunk(chunkCoord, worldGenerator_.GenerateChunk(chunkCoord));
      loadedDataChunks_.insert(chunkCoord);
    }
  }

  // Remove unneeded data chunks. This data will need to be saved and loaded from disk if persistent worlds are ever
  // desired.
  for (auto it = loadedDataChunks_.begin(); it != loadedDataChunks_.end();) {
    if (!desiredDataChunks.contains(*it)) {
      worldView.RemoveChunk(*it);
      it = loadedDataChunks_.erase(it);
    } else {
      ++it;
    }
  }

  // Update Mesh chunks.
  // Remove unneeded mesh chunks
  for (auto it = loadedChunks_.begin(); it != loadedChunks_.end();) {
    if (!desiredMeshChunks.contains(*it)) {
      mesher_.RequestUnload(*it);
      it = loadedChunks_.erase(it);
    } else {
      ++it;
    }
  }

  loadedChunkList_.clear();
  // Request missing mesh chunks be built.
  for (const auto& chunk : desiredMeshChunks) {
    if (!loadedChunks_.contains(chunk)) {
      mesher_.RequestLoad(chunk);
      loadedChunks_.insert(chunk);
    }
    if (mesher_.ChunkStatus(chunk) == ChunkMeshStatus::MissingDependencies)
      mesher_.RequestLoad(chunk);
    loadedChunkList_.push_back(chunk);
  }
}

std::span<const math::Vec3Int> ChunkStreamer::LoadedChunks() const noexcept {
  return loadedChunkList_;
}

ChunkStreamer::ChunkSet ChunkStreamer::buildChunkSet_(const math::Vec3Int centerPosition, const std::int32_t radius) {
  const std::uint32_t size = (2 * radius + 1) * (2 * radius + 1) * worldStore_.WorldHeight;
  ChunkSet result{};
  result.reserve(size);

  for (int dZ = -radius; dZ <= radius; dZ++)
    for (int dX = -radius; dX <= radius; dX++) {
      const int chunkZ = centerPosition.Z + dZ;
      const int chunkX = centerPosition.X + dX;

      if (chunkZ < 0 || chunkZ >= worldStore_.WorldDepth || chunkX < 0 || chunkX >= worldStore_.WorldWidth)
        continue;

      for (std::int32_t y = 0; y < worldStore_.WorldHeight; y++) {
        const math::Vec3Int chunkCoord{.X = chunkX, .Y = y, .Z = chunkZ};
        result.emplace(chunkCoord);
      }
    }

  return result;
}