#pragma once

namespace engine::events {
struct KeyPressedEvent;
struct KeyReleasedEvent;

class IKeyEventHandler {
public:
  virtual ~IKeyEventHandler() = default;

  virtual void OnKeyPressed(const KeyPressedEvent& event) = 0;
  virtual void OnKeyReleased(const KeyReleasedEvent& event) = 0;
};

struct KeyEventDispatch {
  IKeyEventHandler& Handler;

  void operator()(const KeyPressedEvent& e) const {
    Handler.OnKeyPressed(e);
  }
  void operator()(const KeyReleasedEvent& e) const {
    Handler.OnKeyReleased(e);
  }

  template <typename T> void operator()(const T&) const {}
};

class IMouseEventHandler {
public:
  virtual ~IMouseEventHandler() = default;

  virtual void OnMouseMoved(const MouseMovedEvent& event) = 0;
};

struct MouseEventDispatch {
  IMouseEventHandler& Handler;

  void operator()(const MouseMovedEvent& e) const {
    Handler.OnMouseMoved(e);
  }

  template <typename T> void operator()(const T&) const {}
};

} // namespace engine::events
