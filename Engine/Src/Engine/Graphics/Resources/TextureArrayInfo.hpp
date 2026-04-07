#pragma once

#include <cstdint>
#include <cstddef>

namespace engine::graphics::resources {
struct TextureArrayInfo {
  std::uint32_t Height{};
  std::uint32_t Width{};
  std::uint32_t NumLayers{};
  std::size_t LayerSizeBytes{};
};
} // namespace engine::graphics::resources
