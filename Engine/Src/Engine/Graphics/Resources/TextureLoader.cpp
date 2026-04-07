#include "TextureLoader.hpp"

#include "TextureArrayInfo.hpp"
#include "TextureArrayBuilder.hpp"
#include "Engine/Assets/ImageLoader.hpp"
#include "Engine/Graphics/Handles.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"

namespace engine::graphics::resources {
TextureLoader::TextureLoader(vulkan::Renderer& renderer) : renderer_(renderer) {};

std::expected<TextureHandle, TextureLoadError> TextureLoader::Load(std::string_view path) const noexcept {
  auto image = assets::LoadImage(path);
  if (!image) {
    return std::unexpected(TextureLoadError{.Detail = image.error()});
  }

  auto texture = renderer_.CreateTexture(image->Pixels, image->Width, image->Height);
  if (!texture) {
    const std::string msg(vulkan::ToString(texture.error()));
    return std::unexpected(TextureLoadError{.Detail = msg});
  }

  return *texture;
}

std::expected<TextureHandle, TextureLoadError>
TextureLoader::LoadArray(std::span<const std::string_view> paths) const noexcept {

  auto image = assets::LoadImage(paths[0]);
  if (!image) {
    return std::unexpected(TextureLoadError{.Detail = image.error()});
  }

  const TextureArrayInfo info{.Height = static_cast<std::uint32_t>(image->Height),
                              .Width = static_cast<std::uint32_t>(image->Width),
                              .NumLayers = static_cast<std::uint32_t>(paths.size()),
                              .LayerSizeBytes = image->Pixels.size()};
  auto builder = renderer_.BeginTextureArray(info);
  if (!builder) {
    return std::unexpected(TextureLoadError{.Detail = std::string(vulkan::ToString(builder.error()))});
  }
  builder->Upload(image->Pixels);

  for (size_t i = 1; i < paths.size(); i++) {
    image = assets::LoadImage(paths[i]);
    if (!image) {
      return std::unexpected(TextureLoadError{.Detail = image.error()});
    }
    assert(image->Height == info.Height);
    assert(image->Width == info.Width);
    assert(image->Pixels.size() == info.LayerSizeBytes);
    builder->Upload(image->Pixels);
  }

  auto result = builder->Finalize();
  if (!result) {
    return std::unexpected(TextureLoadError{.Detail = image.error()});
  }
  return *result;
}
} // namespace engine::graphics::resources