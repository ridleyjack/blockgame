#pragma once

#include <string>
#include <vector>

namespace engine::assets {

std::vector<char> ReadBinaryFile(const std::string& path);

}