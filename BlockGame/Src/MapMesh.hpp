#pragma once
#include "Engine/Graphics/Handles.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Vulkan/MeshAllocator.hpp"

namespace engine::graphics::vulkan {
class Renderer;
}

class Map;

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

class MapMesh {
public:
  const engine::graphics::MeshHandle& Mesh() const;
  void Upload(engine::graphics::vulkan::Renderer& renderer);

  void Build(const Map& map);

private:
  engine::graphics::MeshHandle mesh_{};
  std::vector<engine::graphics::Vertex> vertices_{};
  std::vector<uint32_t> indices_{};

  void buildChunk_(const Map& map, std::uint32_t mapZ, std::uint32_t mapY, std::uint32_t mapX);
  void buildVertices_(const BlockFaces& faces, std::uint32_t blockType, float z, float y, float x);
  void buildIndices_(std::uint32_t numFaces, std::uint32_t baseVertex);
};
