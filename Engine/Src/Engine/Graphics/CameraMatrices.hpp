#pragma once

#include <glm/glm.hpp>

namespace engine::graphics {

struct CameraMatrices {
  glm::mat4 Projection{1.0f};
  glm::mat4 View{1.0f};
};

} // namespace engine::graphics