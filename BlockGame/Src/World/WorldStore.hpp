#pragma once
#include "Chunk.hpp"

#include <shared_mutex>
#include <unordered_map>

namespace math = engine::math;

class WorldStore {
public:
  using ReadLock = std::shared_lock<std::shared_mutex>;

  static constexpr std::uint32_t WorldWidth{100};
  static constexpr std::uint32_t WorldHeight{3};
  static constexpr std::uint32_t WorldDepth{100};

  WorldStore();

  ReadLock AcquireReadLock() const;
  Chunk* TryGetChunk(math::Vec3Int chunkPosition);

  void StoreChunk(math::Vec3Int chunkPosition, Chunk&& chunk);
  void RemoveChunk(math::Vec3Int chunkPosition);

private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<math::Vec3Int, Chunk, math::Vec3IntHash> chunks_;
};
