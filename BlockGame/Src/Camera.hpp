#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
  Camera();

  glm::vec3 Forward() const noexcept;
  glm::vec3 Right() const noexcept;

  void Move(const glm::vec3& delta) noexcept;
  void Rotate(float deltaX, float deltaY) noexcept;

  glm::mat4 View() const noexcept;

private:
  static constexpr glm::vec3 worldUp_{0.0f, 1.0f, 0.0f};

  float yaw_{-90.0f};
  float pitch_{};

  glm::vec3 position_{};
  glm::vec3 front_{};

  glm::vec3 right_{};
  glm::vec3 up_{};

  void updateVectors_() noexcept;
};
