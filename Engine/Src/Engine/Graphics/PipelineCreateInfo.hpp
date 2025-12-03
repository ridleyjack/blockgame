#pragma once
#include <string>

namespace engine::graphics {
struct PipelineCreateInfo {
  std::string VertexShaderFile{};
  std::string FragmentShaderFile{};
};
} // namespace engine::graphics