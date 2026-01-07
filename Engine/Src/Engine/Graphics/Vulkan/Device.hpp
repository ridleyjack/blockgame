#pragma once

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

namespace engine::graphics::vulkan {

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

struct AllocatedBuffer {
  VkBuffer Buffer{VK_NULL_HANDLE};
  VkDeviceMemory Memory{VK_NULL_HANDLE};
};

class Device {
public:
  explicit Device(Context& context);
  ~Device();

  Device(const Device&) = delete;
  Device& operator=(const Device&) = delete;

  VkDevice Logical() const noexcept;
  VkPhysicalDevice Physical() const noexcept;
  VkQueue GraphicsQueue() const noexcept;
  VkQueue PresentQueue() const noexcept;

  // FindQueueFamilies returns a struct containing the index of a GraphicsFamily and a PresentFamily, preference is
  // given for a family that supports both. Supported families are usually determined by hardware support.
  QueueFamilyIndices FindQueueFamilies() const;
  SwapChainSupportDetails QuerySwapChainSupport() const;

  uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
  VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features) const;

  AllocatedBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const;
  void DestroyBuffer(const AllocatedBuffer& buffer) const noexcept;
  void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const;

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
} // namespace engine::graphics::vulkan
