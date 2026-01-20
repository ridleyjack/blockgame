#pragma once

namespace engine::graphics {
struct TextureArrayInfo {
  std::uint32_t Height{};
  std::uint32_t Width{};
  std::uint32_t NumLayers{};
  std::size_t LayerSizeBytes{};
};
} // namespace engine::graphics
