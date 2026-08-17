#pragma once

#include "WorldStore.hpp"
#include "Containers/ThreadSafeQueue.hpp"
#include "Engine/Math/Vec3Int.hpp"

#include <vector>
#include <span>
#include <unordered_set>

class WorkerPool;
namespace math = engine::math;

class ChunkMesher;
class WorldGenerator;

class ChunkStreamer {
public:
  static constexpr std::size_t LoadRadius = 12;

  ChunkStreamer(WorkerPool& workerPool, WorldStore& worldStore, WorldGenerator& generator, ChunkMesher& mesher);

  void Update(math::Vec3Int playerChunk);

  std::span<const math::Vec3Int> LoadedChunks() const noexcept;

private:
  using ChunkSet = std::unordered_set<math::Vec3Int, math::Vec3IntHash>;

  struct ChunkDataBuildResult {
    math::Vec3Int Coord{};
    Chunk Chunk{};
  };

  WorkerPool& workerPool_;

  WorldStore& worldStore_;
  WorldGenerator& worldGenerator_;
  ChunkMesher& mesher_;

  ChunkSet loadedChunks_{};
  std::vector<math::Vec3Int> loadedChunkList_{};

  ChunkSet loadedDataChunks_{};
  ChunkSet loadingDataChunks_{};

  math::Vec3Int lastPlayerChunk_{-1, -1, -1};

  ThreadSafeQueue<ChunkDataBuildResult> dataChunkResultQueue_{};

  ChunkSet buildChunkSet_(math::Vec3Int centerPosition, std::int32_t radius) const;

  bool enqueueDataChunkBuild_(math::Vec3Int chunkCoord);
  void retryMissingMeshes_() const;
};
