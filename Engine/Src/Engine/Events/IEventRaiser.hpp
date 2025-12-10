#pragma once

#include "Events.hpp"

namespace engine::events {

class IEventRaiser {
public:
  virtual ~IEventRaiser() = default;
  virtual void RaiseEvent(const Event& event) = 0;
};

} // namespace engine::events