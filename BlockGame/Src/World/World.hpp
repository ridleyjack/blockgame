#pragma once

#include "ChunkStreamer.hpp"
#include "ChunkMesher.hpp"
#include "WorldGenerator.hpp"
#include "BlockRegistry.hpp"

#include "Engine/Graphics/Vulkan/Renderer.hpp"
#include "Engine/Math/Frustum.hpp"
#include "Engine/Math/Vec3Int.hpp"

#include <glm/vec3.hpp>

#include <cstdint>
#include <optional>
#include <span>

namespace gfx = engine::graphics;
namespace vlk = gfx::vulkan;
namespace math = engine::math;

struct ChunkMesh;

struct FogShaderData {
  alignas(16) glm::vec3 CameraWorldPos{};
  float FogDensity{};

  alignas(16) glm::vec3 FogColor{};
  float FogStart{};
};

class World {
public:
  struct BlockHit {
    math::Vec3Int Position{};
    BlockType BlockType{};
  };

  static constexpr std::uint32_t WorldWidth{100};
  static constexpr std::uint32_t WorldHeight{3};
  static constexpr std::uint32_t WorldDepth{100};

  World(vlk::Renderer& renderer, gfx::MaterialHandle material, gfx::Color fogColor);
  ~World();

  void Update(math::Vec3Int playerPosition);

  void Upload(glm::vec3 cameraWorldPosition) const;
  void Draw(math::Frustum frustum);

  BlockType GetBlock(math::Vec3Int worldBlockPos);
  void SetBlock(math::Vec3Int worldBlockPos, BlockType blockType);

  std::optional<BlockHit> RaycastBlock(glm::vec3 origin, glm::vec3 direction, float maxDistance);

  std::span<const math::Vec3Int> LoadedChunks() const noexcept;
  std::optional<gfx::MeshHandle> Mesh(const math::Vec3Int& chunkCoord);

  math::AABB ChunkBounds(math::Vec3Int chunkCoord) const noexcept;

private:
  vlk::Renderer& renderer_;
  gfx::PipelineHandle pipeline_;
  gfx::ShaderDataHandle<FogShaderData> fogShaderData_;
  gfx::MaterialHandle blockMaterial_;
  gfx::Color fogColor_;

  WorldStore worldStore_;
  WorldGenerator worldGenerator_;
  BlockRegistry blockRegistry_;
  ChunkMesher chunkMesher_;
  ChunkStreamer chunkStreamer_;
};
