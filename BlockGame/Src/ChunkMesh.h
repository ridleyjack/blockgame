#pragma once
#include "Engine/Graphics/Handles.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Vulkan/MeshAllocator.hpp"

namespace engine::graphics::vulkan {
class Renderer;
}

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

class ChunkMesh {
public:
  ChunkMesh();

  const engine::graphics::MeshHandle& Mesh() const;
  void Upload(engine::graphics::vulkan::Renderer& renderer);

private:
  engine::graphics::MeshHandle mesh_{};
  std::vector<engine::graphics::Vertex> vertices_{};
  std::vector<uint32_t> indices_{};

  void build_();
  void buildVertices_(const BlockFaces& faces, float z, float y, float x);
  void buildIndices_(std::uint32_t numFaces, std::uint32_t baseVertex);
};
