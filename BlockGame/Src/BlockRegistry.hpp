#pragma once

#include <array>
#include <cstdint>

enum class BlockTexture : std::uint8_t;

// Temporary: BlockType enum values used to index blockDef in array.
enum class BlockType : std::uint8_t {
  Dirt = 0,
  Grass,
  Sand,
  Snow,
  Ice,
  Stone,
  Count,
};
constexpr std::size_t BlockTypeCount = static_cast<std::size_t>(BlockType::Count);

enum BlockFace : std::uint8_t {
  Front = 0,
  Back,
  Top,
  Bottom,
  Left,
  Right,
};

class BlockRegistry {
public:
  struct BlockDef {
    std::array<BlockTexture, 6> FaceTextures{};
  };

  BlockRegistry();

  BlockDef& GetBlockDef(BlockType blockType);

private:
  std::array<BlockDef, BlockTypeCount> blockDefs_{};
};
