#pragma once

#include <string>
#include <vector>

namespace Engine::Assets {

std::vector<char> ReadBinaryFile(const std::string& path);

}