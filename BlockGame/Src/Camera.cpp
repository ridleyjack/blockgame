#include "Camera.h"

Camera::Camera() : position_(0.0f, 0.0f, 3.0f) {
  updateVectors_();
}

glm::vec3 Camera::Forward() const noexcept {
  return front_;
}

glm::vec3 Camera::Right() const noexcept {
  return right_;
}

void Camera::Move(const glm::vec3& delta) noexcept {
  position_ += delta;
}
void Camera::Rotate(const float deltaX, const float deltaY) noexcept {
  yaw_ += deltaX;
  pitch_ += deltaY;
  if (pitch_ > 89.0f)
    pitch_ = 89.0f;
  if (pitch_ < -89.0f)
    pitch_ = -89.0f;

  updateVectors_();
}

glm::mat4 Camera::View() const noexcept {
  assert(glm::length(front_) > 0.0f);
  return glm::lookAt(position_, position_ + front_, up_);
}

void Camera::updateVectors_() noexcept {
  glm::vec3 front;
  front.x = static_cast<float>(cos(glm::radians(yaw_)) * cos(glm::radians(pitch_)));
  front.y = static_cast<float>(sin(glm::radians(pitch_)));
  front.z = static_cast<float>(sin(glm::radians(yaw_)) * cos(glm::radians(pitch_)));
  front_ = glm::normalize(front);

  right_ = glm::normalize(glm::cross(front_, worldUp_));
  up_ = glm::normalize(glm::cross(right_, front_));
}