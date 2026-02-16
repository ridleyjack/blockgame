#pragma once
#include "Grid3D.hpp"
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
  std::vector<engine::graphics::Vertex> Vertices{};
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

class ChunkMeshes {
public:
  explicit ChunkMeshes(Map& map);

  const ChunkMesh& Mesh(const math::Vec3Int& mapCoord) const;

  void BuildAll(vlk::Renderer& renderer);
  void BuildChunk(vlk::Renderer& renderer, const math::Vec3Int& mapCoord);

private:
  Map& map_;
  Grid3D<ChunkMesh> meshes_{0, 0, 0, {}};

  void buildChunk_(const math::Vec3Int& mapCoord);
  void buildVertices_(ChunkMesh& mesh, const BlockFaces& faces, std::uint32_t blockType, float z, float y, float x);
  void buildIndices_(ChunkMesh& mesh, std::uint32_t numFaces, std::uint32_t baseVertex);
  void upload_(vlk::Renderer& renderer, ChunkMesh& mesh);
};
