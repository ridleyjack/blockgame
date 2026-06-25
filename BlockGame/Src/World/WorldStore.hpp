#pragma once
#include "Chunk.hpp"

#include <optional>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace math = engine::math;

class WorldStore {
public:
  using ReadLock = std::shared_lock<std::shared_mutex>;
  using WriteLock = std::unique_lock<std::shared_mutex>;

  // WorldToChunkPosition returns the coordinate containing the block at worldPosition.
  static constexpr math::Vec3Int WorldToChunkPosition(const math::Vec3Int worldPosition) {
    assert(worldPosition.X >= 0 && worldPosition.Y >= 0 && worldPosition.Z >= 0);
    return {.X = worldPosition.X / static_cast<std::int32_t>(Chunk::ChunkWidth),
            .Y = worldPosition.Y / static_cast<std::int32_t>(Chunk::ChunkHeight),
            .Z = worldPosition.Z / static_cast<std::int32_t>(Chunk::ChunkDepth)};
  }
  // WorldToChunkLocalPosition returns the block coordinate within its chunk for worldPosition.
  static constexpr math::Vec3Int WorldToChunkLocalPosition(const math::Vec3Int worldPosition) {
    assert(worldPosition.X >= 0 && worldPosition.Y >= 0 && worldPosition.Z >= 0);
    return {.X = worldPosition.X % static_cast<std::int32_t>(Chunk::ChunkWidth),
            .Y = worldPosition.Y % static_cast<std::int32_t>(Chunk::ChunkHeight),
            .Z = worldPosition.Z % static_cast<std::int32_t>(Chunk::ChunkDepth)};
  }

  class ReadView {
  public:
    const Chunk* GetChunk(math::Vec3Int chunkPosition) const;
    std::optional<BlockType> TryGetBlockType(math::Vec3Int worldPosition) const;

  private:
    friend class WorldStore;

    ReadView(ReadLock lock, const WorldStore& worldStore);

    ReadLock lock_;
    const WorldStore& worldStore_;
  };

  class WriteView {
  public:
    Chunk* GetChunk(math::Vec3Int chunkPosition) const;

    void StoreChunk(math::Vec3Int chunkPosition, Chunk&& chunk);
    void RemoveChunk(math::Vec3Int chunkPosition);

  private:
    friend class WorldStore;

    WriteView(WriteLock lock, WorldStore& worldStore);

    WriteLock lock_;
    WorldStore& worldStore_;
  };

  static constexpr std::uint32_t WorldWidth{100};
  static constexpr std::uint32_t WorldHeight{3};
  static constexpr std::uint32_t WorldDepth{100};

  WorldStore() = default;

  ReadView AcquireReadView() const;
  WriteView AcquireWriteView();

private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<math::Vec3Int, Chunk, math::Vec3IntHash> chunks_;
};
