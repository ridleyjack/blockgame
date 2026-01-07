#pragma once

#include <expected>
#include <filesystem>
#include <vector>

namespace engine::assets {

class ImageLoader {
public:
  ImageLoader() = default;
  ~ImageLoader() = default;

  ImageLoader(const ImageLoader&) = delete;
  ImageLoader& operator=(const ImageLoader&) = delete;

  ImageLoader(ImageLoader&& other) noexcept = delete;
  ImageLoader& operator=(ImageLoader&& other) = delete;

  std::expected<void, std::string> Load(const std::filesystem::path& path);

  std::span<const std::byte> Data() const noexcept;

  int Width() const noexcept;
  int Height() const noexcept;
  int Channels() const noexcept;

private:
  std::vector<std::byte> pixels_{};
  int width_{}, height_{};
};
} // namespace engine::assets