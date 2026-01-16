#include "Window.hpp"

#include "Events/Events.hpp"
#include "Events/IEventRaiser.hpp"

#include <cassert>
#include <iostream>

namespace engine {

Window::Window(const WindowConfig& config, events::IEventRaiser& eventRaiser)
    : config_{config}, eventRaiser_(eventRaiser) {}

Window::~Window() {
  Destroy();
}

void Window::Create() {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, config_.IsResizable ? GLFW_TRUE : GLFW_FALSE);

  handle_ = glfwCreateWindow(config_.Width, config_.Height, config_.Title.c_str(), nullptr, nullptr);
  if (!handle_) {
    glfwTerminate();
    throw std::runtime_error("Failed to create glfw window.");
  }
  glfwMakeContextCurrent(handle_);
  glfwSetWindowUserPointer(handle_, this);

  // Keyboard Input
  glfwSetKeyCallback(handle_,
                     [](GLFWwindow* handle, const int key, const int scancode, const int action, const int mods) {
                       const Window& window = *static_cast<Window*>(glfwGetWindowUserPointer(handle));

                       switch (action) {
                       case GLFW_PRESS:
                       case GLFW_REPEAT: {
                         events::KeyPressedEvent event{.Keycode = key, .IsRepeat = action == GLFW_REPEAT};
                         window.eventRaiser_.RaiseEvent(event);
                         break;
                       }
                       case GLFW_RELEASE: {
                         events::KeyReleasedEvent event(key);
                         window.eventRaiser_.RaiseEvent(event);
                         break;
                       }
                       default:;
                       }
                     });

  // Mouse Input
  glfwSetInputMode(handle_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(handle_, [](GLFWwindow* handle, const double x, const double y) {
    const Window& window = *static_cast<Window*>(glfwGetWindowUserPointer(handle));
    events::MouseMovedEvent event(x, y);
    window.eventRaiser_.RaiseEvent(event);
  });
}

void Window::Destroy() {
  if (handle_) {
    glfwDestroyWindow(handle_);
    handle_ = nullptr;
  }
}

int Window::GetFramebufferWidth() const {
  int width{};
  glfwGetFramebufferSize(handle_, &width, nullptr);
  return width;
}

int Window::GetFramebufferHeight() const {
  int height{};
  glfwGetFramebufferSize(handle_, nullptr, &height);
  return height;
}

bool Window::ShouldClose() const {
  return glfwWindowShouldClose(handle_) != 0;
}

GLFWwindow* Window::GetHandle() const {
  return handle_;
}

} // namespace engine