#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace engine::assets {

std::optional<std::vector<std::uint32_t>> ReadBinaryFile(const std::filesystem::path& path);
} // namespace engine::assets
