#pragma once
#include "Chunk.hpp"

#include <optional>
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
  // TryGetChunk returns a pointer to a chunk without any locks.
  Chunk* TryGetChunk(math::Vec3Int chunkPosition);
  // TryGetBlockType returns the block type at the requested position if the chunk is stored or no value if the chunk is
  // not yet stored.
  std::optional<BlockType> TryGetBlockType(math::Vec3Int worldPosition);

  void StoreChunk(math::Vec3Int chunkPosition, Chunk&& chunk);
  void RemoveChunk(math::Vec3Int chunkPosition);

private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<math::Vec3Int, Chunk, math::Vec3IntHash> chunks_;
};
