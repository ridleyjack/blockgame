#include "ImageLoader.hpp"

#include "stb_image.h"

#include <cstring>

constexpr int outputChannels{STBI_rgb_alpha};

namespace engine::assets {

std::expected<void, std::string> ImageLoader::Load(const std::filesystem::path& path) {
  int channels;
  stbi_set_flip_vertically_on_load(true);
  stbi_uc* data = stbi_load(path.c_str(), &width_, &height_, &channels, outputChannels);
  if (!data) {
    return std::unexpected{"Failed to load image: " + path.string() + ": " + stbi_failure_reason()};
  }

  const size_t size = static_cast<size_t>(width_) * height_ * outputChannels;
  pixels_.resize(size);
  std::memcpy(pixels_.data(), data, size);

  stbi_image_free(data);
  return {};
}

std::span<const std::byte> ImageLoader::Data() const noexcept {
  return pixels_;
}

int ImageLoader::Width() const noexcept {
  return width_;
}

int ImageLoader::Height() const noexcept {
  return height_;
}

int ImageLoader::Channels() const noexcept {
  return outputChannels;
}
} // namespace engine::assets