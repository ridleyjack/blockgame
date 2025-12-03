#include "Window.hpp"

#include <cassert>
#include <iostream>

namespace engine {

Window::Window(const WindowConfig& config) : config_{config} {}

Window::~Window() {
  Destroy();
}

void Window::Create() {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  handle_ = glfwCreateWindow(config_.Width, config_.Height, config_.Title.c_str(), nullptr, nullptr);
  if (!handle_) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    assert(false);
  }
  glfwMakeContextCurrent(handle_);
}
void Window::Destroy() {
  if (handle_) {
    glfwDestroyWindow(handle_);
    handle_ = nullptr;
  }
  glfwTerminate();
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