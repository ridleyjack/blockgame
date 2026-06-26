#pragma once

namespace engine::events {

struct FramebufferResizedEvent;
struct KeyPressedEvent;
struct KeyReleasedEvent;
struct MouseMovedEvent;
struct MouseButtonPressedEvent;
struct MouseButtonReleasedEvent;

class IWindowEventHandler {
public:
  virtual ~IWindowEventHandler() = default;

  virtual void OnFramebufferResized(const FramebufferResizedEvent& event) {};
};

struct WindowEventDispatch {
  IWindowEventHandler& Handler;

  void operator()(const FramebufferResizedEvent& e) const {
    Handler.OnFramebufferResized(e);
  }

  template <typename T> void operator()(const T&) const {}
};

class IKeyEventHandler {
public:
  virtual ~IKeyEventHandler() = default;

  virtual void OnKeyPressed(const KeyPressedEvent& event) {};
  virtual void OnKeyReleased(const KeyReleasedEvent& event) {};
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

  virtual void OnMouseMoved(const MouseMovedEvent& event) {};
  virtual void OnMouseButtonPressed(const MouseButtonPressedEvent& event) {};
  virtual void OnMouseButtonReleased(const MouseButtonReleasedEvent& event) {};
};

struct MouseEventDispatch {
  IMouseEventHandler& Handler;

  void operator()(const MouseMovedEvent& e) const {
    Handler.OnMouseMoved(e);
  }
  void operator()(const MouseButtonPressedEvent& e) const {
    Handler.OnMouseButtonPressed(e);
  }
  void operator()(const MouseButtonReleasedEvent& e) const {
    Handler.OnMouseButtonReleased(e);
  }

  template <typename T> void operator()(const T&) const {}
};

} // namespace engine::events
