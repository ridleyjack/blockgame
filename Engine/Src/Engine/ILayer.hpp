#pragma once

namespace engine {
class ILayer {
public:
  virtual ~ILayer() = default;

  virtual void OnUpdate(float deltaTime) = 0;

  virtual void OnRender() = 0;
};
} // namespace engine