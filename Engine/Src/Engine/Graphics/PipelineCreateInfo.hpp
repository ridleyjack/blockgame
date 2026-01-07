#pragma once

#include "Handles.hpp"

#include <string>

namespace engine::graphics {
struct PipelineCreateInfo {
  RenderPassHandle RenderPass{};
  std::string VertexShaderFile{};
  std::string FragmentShaderFile{};
};
} // namespace engine::graphics