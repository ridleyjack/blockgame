#pragma once

#include "Handles.hpp"

#include <string>

namespace engine::graphics {
enum class PipelineKind : std::uint8_t {
  SolidTexture,
  SolidGeometry
};

struct PipelineCreateInfo {
  PipelineKind Kind{};
  std::string VertexShaderFile{};
  std::string FragmentShaderFile{};
};
} // namespace engine::graphics