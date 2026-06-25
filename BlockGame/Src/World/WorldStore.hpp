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

  static constexpr math::Vec3Int WorldToChunkPosition(math::Vec3Int worldPosition);
  static constexpr math::Vec3Int WorldToChunkLocalPosition(math::Vec3Int worldPosition);

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
