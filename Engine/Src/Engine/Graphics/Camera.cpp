#include "Camera.h"

#include <print>

namespace engine::graphics {

Camera::Camera() : position_(0.0f, 0.0f, 3.0f), front_(0.0f, 0.0f, -1.0f), up_(0.0f, 1.0f, 0.0f) {}

glm::vec3 Camera::Forward() const noexcept {
  return glm::normalize(front_);
}

glm::vec3 Camera::Right() const noexcept {
  return glm::normalize(glm::cross(front_, up_));
}

void Camera::Move(const glm::vec3& delta) noexcept {
  position_ += delta;
}

const glm::mat4 Camera::View() const noexcept {
  assert(glm::length(front_) > 0.0f);
  return glm::lookAt(position_, position_ + front_, up_);
}

glm::mat4 Camera::Projection(const uint32_t width, const uint32_t height) const noexcept {
  const float aspect = static_cast<float>(width) / static_cast<float>(height);
  auto proj = glm::perspectiveRH_ZO(glm::radians(45.0f), aspect, 0.1f, 100.0f);
  proj[1][1] *= -1;
  return proj;
}
} // namespace engine::graphics
