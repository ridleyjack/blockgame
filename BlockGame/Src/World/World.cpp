#include "World.hpp"

World::World(vlk::Renderer& renderer)
    : chunkMesher_(renderer, worldGenerator_, blockRegistry_), chunkStreamer_(worldGenerator_, chunkMesher_) {}

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
  throw std::logic_error("not implemented");
  return std::nullopt;
}

std::span<const std::optional<math::Vec3Int>> World::LoadedChunks() const noexcept {
  return chunkStreamer_.LoadedChunks();
}

const ChunkMesh& World::Mesh(const math::Vec3Int& chunkCoord) const {
  return chunkMesher_.Mesh(chunkCoord);
}
