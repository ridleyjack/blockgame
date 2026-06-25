#include "WorldStore.hpp"

#include <mutex>
#include <stdexcept>

WorldStore::ReadView::ReadView(ReadLock lock, const WorldStore& worldStore)
    : lock_(std::move(lock)), worldStore_(worldStore) {}

math::Vec3Int constexpr WorldStore::WorldToChunkPosition(const math::Vec3Int worldPosition) {
  assert(worldPosition.X >= 0 && worldPosition.Y >= 0 && worldPosition.Z >= 0);
  return {.X = worldPosition.X / static_cast<std::int32_t>(Chunk::ChunkWidth),
          .Y = worldPosition.Y / static_cast<std::int32_t>(Chunk::ChunkHeight),
          .Z = worldPosition.Z / static_cast<std::int32_t>(Chunk::ChunkDepth)};
}

math::Vec3Int constexpr WorldStore::WorldToChunkLocalPosition(const math::Vec3Int worldPosition) {
  assert(worldPosition.X >= 0 && worldPosition.Y >= 0 && worldPosition.Z >= 0);
  return {.X = worldPosition.X % static_cast<std::int32_t>(Chunk::ChunkWidth),
          .Y = worldPosition.Y % static_cast<std::int32_t>(Chunk::ChunkHeight),
          .Z = worldPosition.Z % static_cast<std::int32_t>(Chunk::ChunkDepth)};
}

const Chunk* WorldStore::ReadView::GetChunk(const math::Vec3Int chunkPosition) const {
  const auto it = worldStore_.chunks_.find(chunkPosition);
  if (it == worldStore_.chunks_.end())
    return nullptr;

  return &it->second;
}

std::optional<BlockType> WorldStore::ReadView::TryGetBlockType(const math::Vec3Int worldPosition) const {
  if (worldPosition.X < 0 || worldPosition.Y < 0 || worldPosition.Z < 0)
    return std::nullopt;

  const math::Vec3Int chunkPosition = WorldToChunkPosition(worldPosition);

  if (chunkPosition.X >= WorldWidth || chunkPosition.Y >= WorldHeight || chunkPosition.Z >= WorldDepth)
    return std::nullopt;

  const auto it = worldStore_.chunks_.find(chunkPosition);
  if (it == worldStore_.chunks_.end())
    return std::nullopt;

  return it->second.BlockAt(WorldToChunkLocalPosition(worldPosition));
}

WorldStore::WriteView::WriteView(WriteLock lock, WorldStore& worldStore)
    : lock_(std::move(lock)), worldStore_(worldStore) {}

Chunk* WorldStore::WriteView::GetChunk(const math::Vec3Int chunkPosition) const {
  const auto it = worldStore_.chunks_.find(chunkPosition);
  if (it == worldStore_.chunks_.end())
    return nullptr;

  return &it->second;
}

void WorldStore::WriteView::StoreChunk(const math::Vec3Int chunkPosition, Chunk&& chunk) {
  worldStore_.chunks_.insert_or_assign(chunkPosition, std::move(chunk));
}

void WorldStore::WriteView::RemoveChunk(const math::Vec3Int chunkPosition) {
  worldStore_.chunks_.erase(chunkPosition);
}

WorldStore::ReadView WorldStore::AcquireReadView() const {
  return ReadView{ReadLock{mutex_}, *this};
}

WorldStore::WriteView WorldStore::AcquireWriteView() {
  return WriteView{WriteLock{mutex_}, *this};
}
