#include "File.hpp"

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

namespace engine::assets {

std::vector<char> ReadBinaryFile(const std::string& path) {
  std::ifstream file(path, std::ios::ate | std::ios::binary);

  if (!file) {
    std::ostringstream errMsg{};
    errMsg << "Could not open file: " << path;
    if (file.bad()) {
      errMsg << " (badbit set)";
    }
    if (file.fail()) {
      errMsg << " (failbit set)";
    }
    if (errno != 0) {
      errMsg << " - System error: " << std::strerror(errno);
    }
    throw std::runtime_error(errMsg.str());
  }

  size_t size = file.tellg();
  file.seekg(0);

  std::vector<char> buffer(size);
  file.read(buffer.data(), static_cast<std::streamsize>(size));
  return buffer;
}

} // namespace engine::assets