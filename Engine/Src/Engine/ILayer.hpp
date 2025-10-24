#pragma once

#include "IEvent.hpp"

namespace engine {
class ILayer {
public:
  virtual void OnEvent(IEvent& event) {};

  virtual void OnUpdate(float deltaTime) {};

  virtual void OnRender() {};
};
} // namespace Engine