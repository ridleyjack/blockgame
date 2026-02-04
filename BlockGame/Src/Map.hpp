#pragma once
#include "Grid3D.hpp"

#include <filesystem>

constexpr std::uint32_t ChunkWidth{16};
constexpr std::uint32_t ChunkHeight{16};
constexpr std::uint32_t ChunkDepth{16};

struct Chunk {
  Grid3D<std::uint8_t> Blocks{ChunkDepth, ChunkHeight, ChunkWidth, 0};
};

class Map {
public:
  Map(std::uint32_t width, std::uint32_t height, std::uint32_t depth);

  const Chunk& GetChunk(std::uint32_t z, std::uint32_t y, std::uint32_t x) const noexcept;

  std::uint32_t Width() const noexcept;
  std::uint32_t Height() const noexcept;
  std::uint32_t Depth() const noexcept;

private:
  std::uint32_t width_{};
  std::uint32_t height_{};
  std::uint32_t depth_{};

  Grid3D<Chunk> chunks_{0, 0, 0, {}};

  void fillChunk_(std::uint32_t mapZ, std::uint32_t mapY, std::uint32_t mapX) noexcept;
};
