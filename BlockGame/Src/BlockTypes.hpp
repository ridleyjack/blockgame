#pragma once

#include <cstddef>
#include <cstdint>

enum class BlockTexture : std::uint8_t {
  Dirt = 0,
  DirtGrass,
  GrassTop,
  DirtSand,
  DirtSnow,
  Sand,
  Snow,
  Ice,
  Stone,
  StoneDirt,
  StoneGrass,
  StoneSnow,
  Count,
};

// Temporary: BlockType enum values used to index blockDef in array in BlockRegistry.
enum class BlockType : std::uint8_t {
  Air = 0,
  Dirt,
  Grass,
  Sand,
  Snow,
  Ice,
  Stone,
  Count,
};
constexpr std::size_t BlockTypeCount = static_cast<std::size_t>(BlockType::Count);
