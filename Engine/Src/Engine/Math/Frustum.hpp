#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace engine::math {

struct AABB {
  glm::vec3 Min{};
  glm::vec3 Max{};
};

struct Frustum {
  struct Plane {
    glm::vec3 Normal = {0.f, 1.f, 0.f};
    float Distance = 0.f;
  };

  Plane Left;
  Plane Right;
  Plane Bottom;
  Plane Top;
  Plane NearPlane;
  Plane FarPlane;

  static Frustum FromViewProjection(const glm::mat4& projectionView);
  bool Intersects(const AABB& box) const noexcept;
};
} // namespace engine::math