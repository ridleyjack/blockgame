#pragma once

#include "Engine/Math/Vec3Int.hpp"

#include <optional>
#include <vector>

namespace math = engine::math;

class ChunkMesher;
class Map;

class ChunkStreamer {
public:
  static constexpr std::size_t LoadRadius = 12;
  ChunkStreamer(Map& map, ChunkMesher& mesher);

  void Update(math::Vec3Int playerChunk);

  const std::vector<std::optional<math::Vec3Int>>& LoadedChunks() const noexcept;

private:
  Map& map_;
  ChunkMesher& mesher_;
  std::vector<std::optional<math::Vec3Int>> loadedChunks_{};
};