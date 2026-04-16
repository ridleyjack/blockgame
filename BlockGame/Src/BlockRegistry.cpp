#include "BlockRegistry.hpp"

#include "TextureRegistry.hpp"

BlockRegistry::BlockRegistry() {
  constexpr auto toIndex = [](BlockType value) noexcept -> std::size_t { return static_cast<std::size_t>(value); };

  // Dirt.
  blockDefs_[toIndex(BlockType::Dirt)].FaceTextures.fill(BlockTexture::Dirt);

  // Grass
  auto& faces = blockDefs_[toIndex(BlockType::Grass)].FaceTextures;
  faces.fill(BlockTexture::DirtGrass);
  faces[Top] = BlockTexture::GrassTop;
  faces[Bottom] = BlockTexture::Dirt;

  // Sand.
  blockDefs_[toIndex(BlockType::Sand)].FaceTextures.fill(BlockTexture::Sand);

  // Snow.
  blockDefs_[toIndex(BlockType::Snow)].FaceTextures.fill(BlockTexture::Snow);

  // Ice.
  blockDefs_[toIndex(BlockType::Ice)].FaceTextures.fill(BlockTexture::Ice);

  // Stone.
  blockDefs_[toIndex(BlockType::Stone)].FaceTextures.fill(BlockTexture::Stone);
}

BlockRegistry::BlockDef& BlockRegistry::GetBlockDef(BlockType blockType) {
  return blockDefs_[static_cast<std::size_t>(blockType)];
}