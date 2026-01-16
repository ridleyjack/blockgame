#pragma once
#include <variant>

namespace engine::events {

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

using Event = std::variant<KeyPressedEvent, KeyReleasedEvent, MouseMovedEvent>;

} // namespace engine::events
