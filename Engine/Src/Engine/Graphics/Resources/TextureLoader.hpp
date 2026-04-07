#pragma once

#include <expected>
#include <string>
#include <span>

namespace engine::graphics {
struct TextureHandle;

namespace vulkan {
class Renderer;
}

namespace resources {

struct TextureLoadError {
  // TODO: Error enum.
  std::string Detail{};
};

class TextureLoader {
public:
  explicit TextureLoader(vulkan::Renderer& renderer);
  std::expected<TextureHandle, TextureLoadError> Load(std::string_view path) const noexcept;
  std::expected<TextureHandle, TextureLoadError> LoadArray(std::span<const std::string_view> paths) const noexcept;

private:
  vulkan::Renderer& renderer_;
};
} // namespace resources
} // namespace engine::graphics