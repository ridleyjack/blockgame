#include "File.hpp"

#include <fstream>
#include <limits>

namespace fs = std::filesystem;

namespace engine::assets {

std::optional<std::vector<std::uint32_t>> ReadBinaryFile(const fs::path& path) {

  std::error_code err;

  if (!fs::exists(path, err) || err) {
    return {};
  }
  if (!fs::is_regular_file(path, err) || err) {
    return {};
  }

  const uintmax_t size = fs::file_size(path, err);
  if (err)
    return {};

  if (size % sizeof(std::uint32_t) != 0)
    return {};

  if (size > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max()))
    return {};

  std::ifstream file(path, std::ios::binary);
  if (!file)
    return {};

  std::vector<std::uint32_t> buffer(size / sizeof(std::uint32_t));

  if (!file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(size)))
    return {};

  return buffer;
}

} // namespace engine::assets
