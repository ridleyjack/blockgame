#pragma once
#include "Containers/Grid3D.hpp"
#include "Containers/ThreadSafeQueue.hpp"
#include "Engine/Graphics/Handles.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Vulkan/MeshAllocator.hpp"
#include "Engine/Math/Vec3Int.hpp"

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
  engine::graphics::MeshHandle Mesh{};
  std::vector<gfx::Vertex> Vertices{};
  std::vector<std::uint32_t> Indices{};

  bool HasVertices() const noexcept {
    return !Vertices.empty();
  }
};

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

struct ChunkBuildJob {
  math::Vec3Int Coord{};
  std::uint64_t Generation{};
};

struct ChunkBuildResult {
  math::Vec3Int Coord{};
  ChunkMesh Mesh{};
  ChunkMeshStatus Status{};
  std::uint64_t Generation{};
};

class WorldStore;

class ChunkMesher {
public:
  ChunkMesher(vlk::Renderer& renderer, WorldStore& worldStore, BlockRegistry& blockRegistry);
  ~ChunkMesher();

  const ChunkMesh& Mesh(const math::Vec3Int& mapCoord) const;

  void RequestLoad(const math::Vec3Int& mapCoord);
  void RequestUnload(const math::Vec3Int& mapCoord);

  ChunkMeshStatus ChunkStatus(const math::Vec3Int& mapCoord);

  void Update();

private:
  struct ChunkMeshSlot {
    ChunkMesh Mesh{};
    ChunkMeshStatus Status{};
    bool Wanted{};
    std::uint64_t Generation{};
  };

  vlk::Renderer& renderer_;
  WorldStore& worldStore_;
  Grid3D<ChunkMeshSlot> meshes_{0, 0, 0, {}};

  std::vector<std::thread> workers_{};
  std::atomic<bool> stop_{};

  ThreadSafeQueue<ChunkBuildJob> buildQueue_{};
  ThreadSafeQueue<ChunkBuildResult> resultQueue_{};

  BlockRegistry& blockRegistry_;

  void startWorkers_(std::uint32_t count);
  void stopWorkers_();
  void workerLoop_();

  void enqueueBuild_(math::Vec3Int mapCoord);

  std::optional<ChunkMesh> buildChunk_(math::Vec3Int chunkCoord);
  void buildVertices_(ChunkMesh& mesh, const BlockFaces& faces, std::uint32_t blockType, float z, float y, float x);
  void buildIndices_(ChunkMesh& mesh, std::uint32_t baseVertex, std::uint32_t numFaces);
};
