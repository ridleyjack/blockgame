#include "ImageLoader.hpp"

#include "stb_image.h"

#include <stdexcept>

namespace Engine::Assets {

ImageLoader::~ImageLoader() {
  stbi_image_free(pixels_);
}

void ImageLoader::Load(const std::string& path) {
  pixels_ = stbi_load(path.c_str(), &width_, &height_, &channels_, STBI_rgb_alpha);
  if (!pixels_) {
    throw std::runtime_error("failed to load texture image!");
  }
}

const unsigned char* ImageLoader::Data() const {
  return pixels_;
}

int ImageLoader::Width() const {
  return width_;
}

int ImageLoader::Height() const {
  return height_;
}

int ImageLoader::Channels() const {
  return channels_;
}
} // namespace Engine::Assets