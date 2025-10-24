#pragma once

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

namespace Engine {

class Context;

struct QueueFamilyIndices {
  std::optional<uint32_t> GraphicsFamily;
  std::optional<uint32_t> PresentFamily;

  bool IsComplete() const {
    return GraphicsFamily.has_value() && PresentFamily.has_value();
  }
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities{};
  std::vector<VkSurfaceFormatKHR> formats{};
  std::vector<VkPresentModeKHR> presentModes{};
};

class Device {
public:
  explicit Device(Context& context);
  ~Device();

  Device(const Device&) = delete;
  Device& operator=(const Device&) = delete;

  void Init();

  VkDevice Logical() const;
  VkPhysicalDevice Physical() const;
  VkQueue GraphicsQueue() const;
  VkQueue PresentQueue() const;

  // FindQueueFamilies returns a struct containing the index of a GraphicsFamily and a PresentFamily, preference is
  // given for a family that supports both. Supported families are usually determined by hardware support.
  QueueFamilyIndices FindQueueFamilies() const;

  SwapChainSupportDetails QuerySwapChainSupport() const;

  uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

  VkImageView CreateImageView(VkImage image, VkFormat format) const;

private:
  static bool checkDeviceExtensionSupport_(VkPhysicalDevice device);
  static bool isDeviceSuitable_(VkPhysicalDevice device, VkSurfaceKHR surface);
  static QueueFamilyIndices findQueueFamilies_(VkPhysicalDevice device, VkSurfaceKHR surface);
  static SwapChainSupportDetails querySwapChainSupport_(VkPhysicalDevice device, VkSurfaceKHR surface);

  Context& context_;

  VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
  VkDevice device_{VK_NULL_HANDLE};

  VkQueue graphicsQueue_{VK_NULL_HANDLE};
  VkQueue presentQueue_{VK_NULL_HANDLE};

  void pickPhysicalDevice_();
};
} // namespace oc::graphics::vulkan
