#include "World.hpp"

World::World(vlk::Renderer& renderer)
    : chunkMesher_(renderer, worldStore_, blockRegistry_), chunkStreamer_(worldStore_, worldGenerator_, chunkMesher_) {}

void World::Update(math::Vec3Int playerPosition) {
  chunkStreamer_.Update(playerPosition);
  chunkMesher_.Update();
}

BlockType World::GetBlock(math::Vec3Int worldBlockPos) {
  return worldGenerator_.BlockAt(worldBlockPos);
}

void World::SetBlock(math::Vec3Int worldBlockPos, BlockType blockType) {
  throw std::logic_error("not implemented");
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

  float distance = 0.0f;
  while (distance <= maxDistance) {
    if (auto result = worldStore_.TryGetBlockType({.X = blockPos.x, .Y = blockPos.y, .Z = blockPos.z});
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

const ChunkMesh& World::Mesh(const math::Vec3Int& chunkCoord) const {
  return chunkMesher_.Mesh(chunkCoord);
}
