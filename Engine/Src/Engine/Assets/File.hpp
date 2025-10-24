#pragma once

#include <string>
#include <vector>

namespace engine::Assets {

std::vector<char> ReadBinaryFile(const std::string& path);

}