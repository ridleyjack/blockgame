#include "WorldStore.hpp"

#include <mutex>

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

void WorldStore::StoreChunk(const math::Vec3Int chunkPosition, Chunk&& chunk) {
  std::unique_lock lock{mutex_};
  auto [it, inserted] = chunks_.insert_or_assign(chunkPosition, std::move(chunk));
}

void WorldStore::RemoveChunk(const math::Vec3Int chunkPosition) {
  chunks_.erase(chunkPosition);
}
