#include "World.hpp"

#include <array>
#include <limits>

World::World(vlk::Renderer& renderer)
    : chunkMesher_(renderer, worldStore_, blockRegistry_), chunkStreamer_(worldStore_, worldGenerator_, chunkMesher_) {}

void World::Update(const math::Vec3Int playerPosition) {
  chunkStreamer_.Update(playerPosition);
  chunkMesher_.Update();
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

  std::array<math::Vec3Int, 7> chunksToRebuild{chunkCoord};
  std::size_t chunkCount = 1;

  auto addChunk = [&](const math::Vec3Int coord) {
    if (coord.X < 0 || coord.X >= WorldStore::WorldWidth || coord.Y < 0 || coord.Y >= WorldStore::WorldHeight ||
        coord.Z < 0 || coord.Z >= WorldStore::WorldDepth) {
      return;
    }

    chunksToRebuild[chunkCount++] = coord;
  };

  if (localCoord.X == 0)
    addChunk({.X = chunkCoord.X - 1, .Y = chunkCoord.Y, .Z = chunkCoord.Z});
  if (localCoord.X == static_cast<std::int32_t>(Chunk::ChunkWidth) - 1)
    addChunk({.X = chunkCoord.X + 1, .Y = chunkCoord.Y, .Z = chunkCoord.Z});

  if (localCoord.Y == 0)
    addChunk({.X = chunkCoord.X, .Y = chunkCoord.Y - 1, .Z = chunkCoord.Z});
  if (localCoord.Y == static_cast<std::int32_t>(Chunk::ChunkHeight) - 1)
    addChunk({.X = chunkCoord.X, .Y = chunkCoord.Y + 1, .Z = chunkCoord.Z});

  if (localCoord.Z == 0)
    addChunk({.X = chunkCoord.X, .Y = chunkCoord.Y, .Z = chunkCoord.Z - 1});
  if (localCoord.Z == static_cast<std::int32_t>(Chunk::ChunkDepth) - 1)
    addChunk({.X = chunkCoord.X, .Y = chunkCoord.Y, .Z = chunkCoord.Z + 1});

  for (std::size_t i = 0; i < chunkCount; i++) {
    auto chunk = chunksToRebuild[i];
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
