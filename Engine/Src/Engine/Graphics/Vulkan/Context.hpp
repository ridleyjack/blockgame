#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <memory>

namespace engine::graphics::vulkan {

class Device;
class Command;
class SwapChain;
class Sync;

class Context {
public:
  explicit Context(GLFWwindow* window);
  ~Context();

  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;

  GLFWwindow& Window() const noexcept;

  VkInstance Instance() const noexcept;
  VkSurfaceKHR Surface() const noexcept;

  Device& GetDevice() const noexcept;
  SwapChain& GetSwapchain() const noexcept;
  Command& GetCommand() const noexcept;
  Sync& GetSync() const noexcept;

  bool GetFramebufferHasResized() const noexcept;
  void SetFramebufferHasResized(bool value) noexcept;

  bool WindowShouldClose() const noexcept;
  void WaitUntilIdle() const noexcept;

private:
  GLFWwindow* window_{};

  VkDebugUtilsMessengerEXT debugMessenger_{VK_NULL_HANDLE};
  VkInstance instance_{VK_NULL_HANDLE};
  VkSurfaceKHR surface_{VK_NULL_HANDLE};

  std::unique_ptr<Device> device_;
  std::unique_ptr<SwapChain> swapchain_;
  std::unique_ptr<Command> command_;
  std::unique_ptr<Sync> sync_;

  bool framebufferHasResized_{false};

  void createInstance_();
  void setupDebugMessenger_();
  void createSurface_();
};

} // namespace engine::graphics::vulkan
