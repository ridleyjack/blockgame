#include "World.hpp"

#include "Engine/Graphics/SubmitInfo.hpp"
#include "Engine/Graphics/PipelineCreateInfo.hpp"

#include <array>
#include <limits>

World::World(vlk::Renderer& renderer, gfx::MaterialHandle material, gfx::Color fogColor)
    : renderer_(renderer),
      blockMaterial_(material),
      fogColor_(fogColor),
      chunkMesher_(renderer, worldStore_, blockRegistry_),
      chunkStreamer_(worldStore_, worldGenerator_, chunkMesher_) {

  auto shaderDataType = gfx::MakeShaderDataType<FogShaderData>();
  pipeline_ = renderer.CreatePipeline(gfx::PipelineCreateInfo{
      .Kind = gfx::PipelineKind::SolidTexture,
      .VertexShaderFile = "Shaders/terrain.vert.spv",
      .FragmentShaderFile = "Shaders/terrain.frag.spv",
      .ShaderDataSlots = {shaderDataType},
  });

  fogShaderData_ = renderer_.CreateShaderData<FogShaderData>();
}

World::~World() {
  renderer_.DeleteShaderData(fogShaderData_);
  renderer_.DeletePipeline(pipeline_);
}

void World::Update(const math::Vec3Int playerPosition) {
  chunkStreamer_.Update(playerPosition);
  chunkMesher_.Update();
}

void World::Upload(const glm::vec3 cameraWorldPosition) const {
  FogShaderData fog{
      .CameraWorldPos = cameraWorldPosition,
      .FogDensity = 0.008f,
      .FogColor = glm::vec3{fogColor_.R, fogColor_.G, fogColor_.B},
      .FogStart = 16 * 4,
  };
  renderer_.SetShaderData(fogShaderData_, fog);
}

void World::Draw(math::Frustum frustum) {
  auto fogBinding = gfx::BindShaderData(fogShaderData_);

  for (const auto& meshCoords : LoadedChunks()) {
    if (!frustum.Intersects(ChunkBounds(meshCoords)))
      continue;

    const auto meshHandle = Mesh(meshCoords);
    if (meshHandle) {
      renderer_.Submit(gfx::SubmitInfo{
          .Pipeline = pipeline_,
          .Mesh = *meshHandle,
          .Material = blockMaterial_,
          .ShaderData = std::span{&fogBinding, 1},
      });
    }
  }
}

BlockType World::GetBlock(const math::Vec3Int worldBlockPos) {
  return worldGenerator_.BlockAt(worldBlockPos);
}

void World::SetBlock(const math::Vec3Int worldBlockPos, const BlockType blockType) {
  const math::Vec3Int chunkCoord = worldStore_.WorldToChunkPosition(worldBlockPos);
  const math::Vec3Int localCoord = worldStore_.WorldToChunkLocalPosition(worldBlockPos);

  {
    auto writeView = worldStore_.AcquireWriteView();
    Chunk* chunk = writeView.GetChunk(chunkCoord);
    if (chunk == nullptr)
      return;
    if (chunk->BlockAt(localCoord) == blockType)
      return;

    chunk->SetBlock(localCoord, blockType);
  }

  constexpr std::int32_t worldWidth = WorldStore::WorldWidth * Chunk::ChunkWidth;
  constexpr std::int32_t worldHeight = WorldStore::WorldHeight * Chunk::ChunkHeight;
  constexpr std::int32_t worldDepth = WorldStore::WorldDepth * Chunk::ChunkDepth;

  std::array<math::Vec3Int, 8> chunksToRebuild{}; // Max adjacent chunks is 8, includes horizontal.
  std::size_t chunkCount = 0;

  auto addChunk = [&](const math::Vec3Int coord) {
    for (std::size_t i = 0; i < chunkCount; i++) {
      if (chunksToRebuild[i] == coord)
        return;
    }
    assert(chunkCount < chunksToRebuild.size());
    chunksToRebuild[chunkCount++] = coord;
  };

  for (std::int32_t z = -1; z <= 1; z++) {
    for (std::int32_t y = -1; y <= 1; y++) {
      for (std::int32_t x = -1; x <= 1; x++) {
        const math::Vec3Int blockCoord{
            .X = worldBlockPos.X + x,
            .Y = worldBlockPos.Y + y,
            .Z = worldBlockPos.Z + z,
        };

        if (blockCoord.X < 0 || blockCoord.X >= worldWidth || blockCoord.Y < 0 || blockCoord.Y >= worldHeight ||
            blockCoord.Z < 0 || blockCoord.Z >= worldDepth) {
          continue;
        }

        addChunk(worldStore_.WorldToChunkPosition(blockCoord));
      }
    }
  }

  for (std::size_t i = 0; i < chunkCount; i++) {
    const math::Vec3Int chunk = chunksToRebuild[i];
    if (chunkMesher_.ChunkStatus(chunk) == ChunkMeshStatus::Unloaded)
      continue; // No need to rebuild unloaded meshes.
    chunkMesher_.RequestRebuild(chunk);
  }
}

std::optional<World::BlockHit> World::RaycastBlock(glm::vec3 origin, glm::vec3 direction, float maxDistance) {
  glm::ivec3 blockPos = glm::floor(origin);

  constexpr float infinity = std::numeric_limits<float>::infinity();
  auto axisDelta = [](float component) { return component == 0.0f ? infinity : std::abs(1.0f / component); };

  const glm::vec3 deltaDist{
      axisDelta(direction.x),
      axisDelta(direction.y),
      axisDelta(direction.z),
  };

  auto setupAxis = [](float originCoord, int blockCoord, float directionCoord, float delta) {
    struct Axis {
      int Step{};
      float SideDist{};
    };

    if (directionCoord > 0.0f) {
      return Axis{
          .Step = 1,
          .SideDist = (static_cast<float>(blockCoord + 1) - originCoord) * delta,
      };
    }
    if (directionCoord < 0.0f) {
      return Axis{
          .Step = -1,
          .SideDist = (originCoord - static_cast<float>(blockCoord)) * delta,
      };
    }
    return Axis{
        .Step = 0,
        .SideDist = infinity,
    };
  };

  const auto x = setupAxis(origin.x, blockPos.x, direction.x, deltaDist.x);
  const auto y = setupAxis(origin.y, blockPos.y, direction.y, deltaDist.y);
  const auto z = setupAxis(origin.z, blockPos.z, direction.z, deltaDist.z);

  glm::ivec3 step{
      step.x = x.Step,
      step.y = y.Step,
      step.z = z.Step,
  };
  glm::vec3 sideDist{
      sideDist.x = x.SideDist,
      sideDist.y = y.SideDist,
      sideDist.z = z.SideDist,
  };

  const auto worldView = worldStore_.AcquireReadView();

  float distance = 0.0f;
  while (distance <= maxDistance) {
    if (auto result = worldView.TryGetBlockType({.X = blockPos.x, .Y = blockPos.y, .Z = blockPos.z});
        result && *result != BlockType::Air) {
      return BlockHit{
          .Position = {.X = blockPos.x, .Y = blockPos.y, .Z = blockPos.z},
          .BlockType = *result,
      };
    }

    // advance to next voxel boundary
    if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
      blockPos.x += step.x;
      distance = sideDist.x;
      sideDist.x += deltaDist.x;
    } else if (sideDist.y < sideDist.z) {
      blockPos.y += step.y;
      distance = sideDist.y;
      sideDist.y += deltaDist.y;
    } else {
      blockPos.z += step.z;
      distance = sideDist.z;
      sideDist.z += deltaDist.z;
    }
  }

  return std::nullopt;
}

std::span<const math::Vec3Int> World::LoadedChunks() const noexcept {
  return chunkStreamer_.LoadedChunks();
}

std::optional<gfx::MeshHandle> World::Mesh(const math::Vec3Int& chunkCoord) {
  return chunkMesher_.RenderableMesh(chunkCoord);
}

math::AABB World::ChunkBounds(const math::Vec3Int chunkCoord) const noexcept {
  return {
      .Min =
          {
                static_cast<float>(chunkCoord.X * Chunk::ChunkWidth),
                static_cast<float>(chunkCoord.Y * Chunk::ChunkHeight),
                static_cast<float>(chunkCoord.Z * Chunk::ChunkDepth),
                },
      .Max =
          {
                static_cast<float>((chunkCoord.X + 1) * Chunk::ChunkWidth),
                static_cast<float>((chunkCoord.Y + 1) * Chunk::ChunkHeight),
                static_cast<float>((chunkCoord.Z + 1) * Chunk::ChunkDepth),
                },
  };
}
