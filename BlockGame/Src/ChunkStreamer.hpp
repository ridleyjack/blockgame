#pragma once

#include "Engine/Math/Vec3Int.hpp"

#include <optional>
#include <vector>

namespace math = engine::math;

class ChunkMesher;
class WorldGenerator;

class ChunkStreamer {
public:
  static constexpr std::size_t LoadRadius = 12;
  ChunkStreamer(WorldGenerator& generator, ChunkMesher& mesher);

  void Update(math::Vec3Int playerChunk);

  const std::vector<std::optional<math::Vec3Int>>& LoadedChunks() const noexcept;

private:
  WorldGenerator& worldGenerator_;
  ChunkMesher& mesher_;
  std::vector<std::optional<math::Vec3Int>> loadedChunks_{};
};