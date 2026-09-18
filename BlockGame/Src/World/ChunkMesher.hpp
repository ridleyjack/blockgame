#pragma once
#include "WorldStore.hpp"

#include "Containers/Grid3D.hpp"
#include "Containers/ThreadSafeQueue.hpp"
#include "Engine/Graphics/Handles.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Vulkan/MeshAllocator.hpp"
#include "Engine/Math/Vec3Int.hpp"

#include <array>

class WorkerPool;
namespace gfx = engine::graphics;
namespace vlk = gfx::vulkan;
namespace math = engine::math;

namespace engine::graphics::vulkan {
class Renderer;
}

class BlockRegistry;

enum class ChunkMeshStatus : std::uint8_t {
  Unloaded = 0,
  Building,
  Uploaded,
  MissingDependencies,
};

struct ChunkMesh {
  std::vector<gfx::Vertex> Vertices{};
  std::vector<std::uint32_t> Indices{};

  bool HasVertices() const noexcept {
    return !Vertices.empty();
  }
};

struct ChunkBuildResult {
  math::Vec3Int Coord{};
  ChunkMesh Mesh{};
  ChunkMeshStatus Status{};
  std::uint64_t Generation{};
};

class ChunkMesher {
public:
  ChunkMesher(vlk::Renderer& renderer, WorkerPool& workerPool, WorldStore& worldStore, BlockRegistry& blockRegistry);

  std::optional<gfx::MeshHandle> RenderableMesh(math::Vec3Int mapCoord);

  void RequestLoad(math::Vec3Int mapCoord);
  void RequestRebuild(math::Vec3Int mapCoord);
  void RequestUnload(math::Vec3Int mapCoord);

  ChunkMeshStatus ChunkStatus(math::Vec3Int mapCoord);

  std::array<math::Vec3Int, 27> GetRequiredChunks(math::Vec3Int chunkCoord) const;

  void Update();

private:
  using FaceAmbientOcclusion = std::array<std::array<float, 4>, 6>;

  struct BlockFaces {
    bool Front{};
    bool Back{};
    bool Right{};
    bool Left{};
    bool Top{};
    bool Bottom{};

    constexpr std::uint8_t NumEnabled() const noexcept {
      return static_cast<std::uint8_t>(Front + Back + Right + Left + Top + Bottom);
    };
  };

  struct ChunkMeshSlot {
    std::optional<gfx::MeshHandle> VisibleHandle{};
    std::optional<gfx::MeshHandle> PendingHandle{};
    ChunkMeshStatus Status{};
    std::uint64_t Generation{};

    bool Wanted{};
  };

  vlk::Renderer& renderer_;
  WorkerPool& workerPool_;

  WorldStore& worldStore_;
  Grid3D<ChunkMeshSlot> meshes_{0, 0, 0, {}};

  ThreadSafeQueue<ChunkBuildResult> resultQueue_{};

  BlockRegistry& blockRegistry_;

  void enqueueBuild_(math::Vec3Int mapCoord);
  bool buildDependenciesReady_(const WorldStore::ReadView& worldView, math::Vec3Int chunkCoord) const;

  ChunkMesh buildChunk_(const WorldStore::ReadView& worldView, math::Vec3Int chunkCoord);

  void buildSolidBlock_(const WorldStore::ReadView& worldView,
                        ChunkMesh& mesh,
                        math::Vec3Int chunkCoord,
                        math::Vec3Int blockCoord);

  void buildCrossBillboardBlock_(ChunkMesh& mesh, math::Vec3Int chunkCoord, math::Vec3Int blockCoord, BlockType type);

  void buildSolidBlockVertices_(ChunkMesh& mesh,
                                const BlockFaces& faces,
                                const FaceAmbientOcclusion& ambientOcclusion,
                                std::uint32_t blockType,
                                float z,
                                float y,
                                float x);

  void buildIndices_(ChunkMesh& mesh, std::uint32_t baseVertex, std::uint32_t numFaces);
};
