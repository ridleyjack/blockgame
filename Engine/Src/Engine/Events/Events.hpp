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

using Event = std::variant<KeyPressedEvent, KeyReleasedEvent>;

} // namespace engine::events
