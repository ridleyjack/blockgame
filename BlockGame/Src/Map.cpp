#include "Map.hpp"

Map::Map(std::uint32_t width, std::uint32_t height, std::uint32_t depth)
    : width_(width), height_(height), depth_(depth), chunks_(depth, height, width, {}) {

  for (std::uint32_t z = 0; z < depth; z++) {
    for (std::uint32_t y = 0; y < height; y++) {
      for (std::uint32_t x = 0; x < width; x++) {
        fillChunk_(z, y, x);
      }
    }
  }
}
const Chunk& Map::GetChunk(const std::uint32_t z, const std::uint32_t y, const std::uint32_t x) const noexcept {
  return chunks_[z, y, x];
}

std::uint32_t Map::Width() const noexcept {
  return width_;
}

std::uint32_t Map::Depth() const noexcept {
  return depth_;
}

std::uint32_t Map::Height() const noexcept {
  return height_;
}

void Map::fillChunk_(const std::uint32_t mapZ, const std::uint32_t mapY, const std::uint32_t mapX) noexcept {
  Grid3D<std::uint8_t>& blocks = chunks_[mapZ, mapY, mapX].Blocks;

  const std::uint32_t blockVal = mapZ % 4 + 1;
  for (std::uint32_t z = 0; z < blocks.Depth(); z++)
    for (std::uint32_t y = 0; y < blocks.Height(); y++)
      for (std::uint32_t x = 0; x < blocks.Width(); x++) {
        blocks[z, y, x] = blockVal;
      }
  blocks[0, 0, 0] = 0;
}
