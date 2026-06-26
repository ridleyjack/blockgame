#pragma once
#include <variant>

namespace engine::events {

struct FramebufferResizedEvent {
  int Width{};
  int Height{};
};

struct KeyPressedEvent {
  int Keycode{};
  bool IsRepeat{};
};

struct KeyReleasedEvent {
  int Keycode{};
};

struct MouseMovedEvent {
  double X{};
  double Y{};
};

struct MouseButtonPressedEvent {
  int Button{};
};

struct MouseButtonReleasedEvent {
  int Button{};
};

using Event = std::variant<FramebufferResizedEvent,
                           KeyPressedEvent,
                           KeyReleasedEvent,
                           MouseMovedEvent,
                           MouseButtonPressedEvent,
                           MouseButtonReleasedEvent>;

} // namespace engine::events
