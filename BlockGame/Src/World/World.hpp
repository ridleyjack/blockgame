#pragma once

#include "ChunkStreamer.hpp"
#include "ChunkMesher.hpp"
#include "WorldGenerator.hpp"
#include "BlockRegistry.hpp"

#include "Engine/Graphics/Vulkan/Renderer.hpp"
#include "Engine/Math/Vec3Int.hpp"

#include <glm/vec3.hpp>

#include <cstdint>
#include <optional>
#include <span>

namespace gfx = engine::graphics;
namespace vlk = gfx::vulkan;
namespace math = engine::math;

struct ChunkMesh;

class World {
public:
  struct BlockHit {
    math::Vec3Int Position{};
    BlockType BlockType{};
  };

  static constexpr std::uint32_t WorldWidth{100};
  static constexpr std::uint32_t WorldHeight{3};
  static constexpr std::uint32_t WorldDepth{100};

  explicit World(vlk::Renderer& renderer);

  void Update(math::Vec3Int playerPosition);

  BlockType GetBlock(math::Vec3Int worldBlockPos);
  void SetBlock(math::Vec3Int worldBlockPos, BlockType blockType);

  std::optional<BlockHit> RaycastBlock(glm::vec3 origin, glm::vec3 direction, float maxDistance);

  std::span<const math::Vec3Int> LoadedChunks() const noexcept;
  std::optional<gfx::MeshHandle> Mesh(const math::Vec3Int& chunkCoord);

private:
  WorldStore worldStore_;
  WorldGenerator worldGenerator_;
  BlockRegistry blockRegistry_;
  ChunkMesher chunkMesher_;
  ChunkStreamer chunkStreamer_;
};
