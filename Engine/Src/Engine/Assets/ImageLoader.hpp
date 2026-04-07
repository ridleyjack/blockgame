#pragma once

#include <expected>
#include <filesystem>
#include <vector>

namespace engine::assets {

struct ImageData {
  std::vector<std::byte> Pixels{};
  int Width{};
  int Height{};
  int Channels{};
};

std::expected<ImageData, std::string> LoadImage(const std::filesystem::path& path);
} // namespace engine::assets