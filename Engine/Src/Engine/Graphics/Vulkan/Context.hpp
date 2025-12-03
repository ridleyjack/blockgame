#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <memory>
#include <vector>

namespace engine::graphics::vulkan {

class Device;
class Command;
class SwapChain;
class Sync;

namespace config {

constexpr bool EnableValidationLayers{
#ifdef NDEBUG
    false
#else
    true
#endif
};

const std::vector<const char*> ValidationLayers{"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

constexpr uint32_t Width = 800;
constexpr uint32_t Height = 600;
constexpr int MaxFramesInFlight = 2;

} // namespace config

class Context {
public:
  explicit Context(GLFWwindow* window);
  ~Context();

  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;

  GLFWwindow& Window() const noexcept;

  VkInstance Instance() const noexcept;
  VkSurfaceKHR Surface() const noexcept;

  Device& GetDevice() noexcept;
  SwapChain& GetSwapchain() noexcept;
  Command& GetCommand() noexcept;
  Sync& GetSync() noexcept;

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
