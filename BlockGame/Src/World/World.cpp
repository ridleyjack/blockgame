#include "World.hpp"

#include <array>

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

  const glm::vec3 deltaDist = glm::abs(glm::vec3{1.0f / direction.x, 1.0f / direction.y, 1.0f / direction.z});

  glm::ivec3 step{};
  glm::vec3 sideDist{};

  // X
  if (direction.x < 0.0f) {
    step.x = -1;
    sideDist.x = (origin.x - static_cast<float>(blockPos.x)) * deltaDist.x;
  } else {
    step.x = 1;
    sideDist.x = (static_cast<float>(blockPos.x + 1) - origin.x) * deltaDist.x;
  }

  // Y
  if (direction.y < 0.0f) {
    step.y = -1;
    sideDist.y = (origin.y - static_cast<float>(blockPos.y)) * deltaDist.y;
  } else {
    step.y = 1;
    sideDist.y = (static_cast<float>(blockPos.y + 1) - origin.y) * deltaDist.y;
  }

  // Z
  if (direction.z < 0.0f) {
    step.z = -1;
    sideDist.z = (origin.z - static_cast<float>(blockPos.z)) * deltaDist.z;
  } else {
    step.z = 1;
    sideDist.z = (static_cast<float>(blockPos.z + 1) - origin.z) * deltaDist.z;
  }

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
