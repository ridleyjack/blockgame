#pragma once

#include "IEvent.hpp"

namespace Engine {
class ILayer {
public:
  virtual void OnEvent(IEvent& event) {};

  virtual void OnUpdate(float deltaTime) {};

  virtual void OnRender() {};
};
} // namespace Engine