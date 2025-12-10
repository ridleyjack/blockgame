#pragma once

#include "Events/Events.hpp"

namespace engine {
class ILayer {
public:
  virtual ~ILayer() = default;
  // virtual void OnEvent(Event& event) {};

  virtual void OnUpdate(float deltaTime) = 0;

  virtual void OnRender() = 0;
};
} // namespace engine