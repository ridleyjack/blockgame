#pragma once
#include "Grid3D.hpp"
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

class Map;

struct ChunkMesh {
  engine::graphics::MeshHandle Mesh{};
  std::vector<gfx::Vertex> Vertices{};
  std::vector<uint32_t> Indices{};

  bool HasVertices() const {
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
    return static_cast<std::int8_t>(Front + Back + Right + Left + Top + Bottom);
  };
};

struct ChunkBuildJob {
  math::Vec3Int Coord{};
};

struct ChunkBuildResult {
  math::Vec3Int Coord{};
  ChunkMesh Mesh{};
};

class ChunkMeshes {
public:
  explicit ChunkMeshes(Map& map);
  ~ChunkMeshes();

  const ChunkMesh& Mesh(const math::Vec3Int& mapCoord) const;

  void BuildAll();
  void BuildChunk(const math::Vec3Int& mapCoord);

  void Update(vlk::Renderer& renderer);

private:
  Map& map_;
  Grid3D<ChunkMesh> meshes_{0, 0, 0, {}};

  std::vector<std::thread> workers_{};
  std::atomic<bool> stop_{};
  ThreadSafeQueue<ChunkBuildJob> buildQueue_{};
  ThreadSafeQueue<ChunkBuildResult> resultQueue_{};

  void startWorkers_(std::uint32_t count);
  void stopWorkers_();
  void workerLoop_();

  ChunkMesh buildChunk_(const math::Vec3Int& mapCoord);
  void buildVertices_(ChunkMesh& mesh, const BlockFaces& faces, std::uint32_t blockType, float z, float y, float x);
  void buildIndices_(ChunkMesh& mesh, std::uint32_t baseVertex, std::uint32_t numFaces);

  void upload_(vlk::Renderer& renderer, ChunkMesh& mesh);
};
