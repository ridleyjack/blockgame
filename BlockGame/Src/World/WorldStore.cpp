#include "WorldStore.hpp"

#include <mutex>
#include <stdexcept>

WorldStore::WorldStore() {}

WorldStore::ReadLock WorldStore::AcquireReadLock() const {
  return ReadLock{mutex_};
}

Chunk* WorldStore::TryGetChunk(const math::Vec3Int chunkPosition) {
  const auto it = chunks_.find(chunkPosition);
  if (it == chunks_.end())
    return nullptr;

  return &it->second;
}
std::optional<BlockType> WorldStore::TryGetBlockType(const math::Vec3Int worldPosition) {
  if (worldPosition.X < 0 || worldPosition.Y < 0 || worldPosition.Z < 0)
    return std::nullopt;

  std::int32_t chunkX = worldPosition.X / static_cast<std::int32_t>(Chunk::ChunkWidth);
  std::int32_t chunkY = worldPosition.Y / static_cast<std::int32_t>(Chunk::ChunkHeight);
  std::int32_t chunkZ = worldPosition.Z / static_cast<std::int32_t>(Chunk::ChunkDepth);

  if (chunkX >= WorldWidth || chunkY >= WorldHeight || chunkZ >= WorldDepth)
    return std::nullopt;

  auto lock = ReadLock{mutex_};
  const auto it = chunks_.find({.X = chunkX, .Y = chunkY, .Z = chunkZ});
  if (it == chunks_.end())
    return std::nullopt;

  return it->second.BlockAt({.X = static_cast<std::int32_t>(worldPosition.X % Chunk::ChunkWidth),
                             .Y = static_cast<std::int32_t>(worldPosition.Y % Chunk::ChunkHeight),
                             .Z = static_cast<std::int32_t>(worldPosition.Z % Chunk::ChunkDepth)});
}

void WorldStore::StoreChunk(const math::Vec3Int chunkPosition, Chunk&& chunk) {
  std::unique_lock lock{mutex_};
  chunks_.insert_or_assign(chunkPosition, std::move(chunk));
}

void WorldStore::RemoveChunk(const math::Vec3Int chunkPosition) {
  std::unique_lock lock{mutex_};
  chunks_.erase(chunkPosition);
}
