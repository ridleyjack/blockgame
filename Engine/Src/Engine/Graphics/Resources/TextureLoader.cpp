#include "TextureLoader.hpp"

#include "TextureArrayInfo.hpp"
#include "TextureArrayBuilder.hpp"
#include "Engine/Assets/ImageLoader.hpp"
#include "Engine/Fatal.hpp"
#include "Engine/Graphics/Handles.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"

#include <cassert>
#include <format>

namespace engine::graphics::resources {
TextureLoader::TextureLoader(vulkan::Renderer& renderer) : renderer_(renderer) {};

std::expected<TextureHandle, TextureLoadError> TextureLoader::Load(std::string_view path) const {
  auto image = assets::LoadImage(path);
  if (!image) {
    return std::unexpected(TextureLoadError{.Detail = image.error()});
  }

  return renderer_.CreateTexture(image->Pixels, image->Width, image->Height);
}

TextureHandle TextureLoader::LoadArray(std::span<const std::string_view> paths) const {

  auto image = assets::LoadImage(paths[0]);
  if (!image) {
    const auto msg = std::format("TextureLoader failed to load image at {} reason:{}", paths[0], image.error());
    Fatal(msg);
  }

  const TextureArrayInfo info{.Height = static_cast<std::uint32_t>(image->Height),
                              .Width = static_cast<std::uint32_t>(image->Width),
                              .NumLayers = static_cast<std::uint32_t>(paths.size()),
                              .LayerSizeBytes = image->Pixels.size()};

  auto builder = renderer_.BeginTextureArray(info);
  builder.Upload(image->Pixels);

  for (size_t i = 1; i < paths.size(); i++) {
    image = assets::LoadImage(paths[i]);
    if (!image) {
      const auto msg = std::format("TextureLoader failed to load image at {} reason:{}", paths[i], image.error());
      Fatal(msg);
    }
    assert(image->Height == info.Height);
    assert(image->Width == info.Width);
    assert(image->Pixels.size() == info.LayerSizeBytes);
    builder.Upload(image->Pixels);
  }

  return builder.Finalize();
}
} // namespace engine::graphics::resources
