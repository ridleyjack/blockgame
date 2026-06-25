#pragma once

#include "BlockTypes.hpp"

#include <array>
#include <cstdint>

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

  const BlockDef& GetBlockDef(BlockType blockType) const;

private:
  std::array<BlockDef, BlockTypeCount> blockDefs_{};
};
