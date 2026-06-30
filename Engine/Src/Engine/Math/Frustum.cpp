#include "Frustum.hpp"

#include <glm/glm.hpp>

namespace engine::math {
Frustum Frustum::FromViewProjection(const glm::mat4& projectionView) {

  auto normalizePlane = [](glm::vec4 p) -> Plane {
    const float length = glm::length(glm::vec3{p});
    return {
        .Normal = glm::vec3{p} / length,
        .Distance = p.w / length,
    };
  };

  const glm::mat4 m = glm::transpose(projectionView);
  return {
      .Left = normalizePlane(m[3] + m[0]),
      .Right = normalizePlane(m[3] - m[0]),
      .Bottom = normalizePlane(m[3] + m[1]),
      .Top = normalizePlane(m[3] - m[1]),

      .NearPlane = normalizePlane(m[2]),
      .FarPlane = normalizePlane(m[3] - m[2]),
  };
}

bool Frustum::Intersects(const AABB& box) const noexcept {

  auto isOnPositiveSide = [](const Plane& plane, const AABB& box) -> bool {
    glm::vec3 positive = box.Min;

    if (plane.Normal.x >= 0.0f)
      positive.x = box.Max.x;
    if (plane.Normal.y >= 0.0f)
      positive.y = box.Max.y;
    if (plane.Normal.z >= 0.0f)
      positive.z = box.Max.z;

    return (glm::dot(plane.Normal, positive) + plane.Distance) >= 0.0f;
  };

  return isOnPositiveSide(Left, box) && isOnPositiveSide(Right, box) && isOnPositiveSide(Bottom, box) &&
         isOnPositiveSide(Top, box) && isOnPositiveSide(NearPlane, box) && isOnPositiveSide(FarPlane, box);
}
} // namespace engine::math