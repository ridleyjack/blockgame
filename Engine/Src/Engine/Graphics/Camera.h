#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine::graphics {

class Camera {
public:
  Camera();

  glm::vec3 Forward() const noexcept;
  glm::vec3 Right() const noexcept;

  void Move(const glm::vec3& delta) noexcept;

  const glm::mat4 View() const noexcept;
  glm::mat4 Projection(uint32_t width, uint32_t height) const noexcept;

private:
  glm::vec3 position_{};
  glm::vec3 front_{};
  glm::vec3 up_{};

  glm::mat4 view_{1.0f};
};

} // namespace engine::graphics