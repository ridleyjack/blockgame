#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <string>

namespace engine {

namespace events {
class IEventRaiser;
}

struct WindowConfig {
  std::string Title{};
  int Width{1280};
  int Height{720};
  bool IsResizable{true};
  bool VSync{true}; // TODO: set via vulkan presentMode.
};

class Window {
public:
  explicit Window(const WindowConfig& config, events::IEventRaiser& eventRaiser);
  ~Window();

  void Create();
  void Destroy();

  int GetFramebufferWidth() const;
  int GetFramebufferHeight() const;

  bool ShouldClose() const;

  GLFWwindow* GetHandle() const;

private:
  WindowConfig config_{};
  GLFWwindow* handle_{nullptr};

  events::IEventRaiser& eventRaiser_;
};

} // namespace engine