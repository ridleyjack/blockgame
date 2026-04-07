#include "ImageLoader.hpp"

#include "stb_image.h"

#include <cstring>

constexpr int outputChannels{STBI_rgb_alpha};

namespace engine::assets {

std::expected<ImageData, std::string> LoadImage(const std::filesystem::path& path) {
  ImageData result{};
  int channels{};

  stbi_set_flip_vertically_on_load(true);
  stbi_uc* data = stbi_load(path.c_str(), &result.Width, &result.Height, &channels, outputChannels);
  if (!data) {
    return std::unexpected{"Failed to load image: " + path.string() + ": " + stbi_failure_reason()};
  }

  const size_t size = static_cast<size_t>(result.Width) * result.Height * outputChannels;
  result.Pixels.resize(size);
  std::memcpy(result.Pixels.data(), data, size);
  stbi_image_free(data);

  return result;
}

} // namespace engine::assets