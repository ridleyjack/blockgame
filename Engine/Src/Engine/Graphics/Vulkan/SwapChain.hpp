#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace engine::graphics::vulkan {

class Context;

class SwapChain {

public:
  explicit SwapChain(Context& context);
  ~SwapChain();

  SwapChain(const SwapChain&) = delete;
  SwapChain& operator=(const SwapChain&) = delete;

  void Cleanup();
  void Recreate();

  void CreateFrameBuffers(VkRenderPass renderPass);

  VkSwapchainKHR Handle() const noexcept;
  VkFormat ImageFormat() const noexcept;
  VkExtent2D Extent() const noexcept;

  std::vector<VkImage> Images() const noexcept;
  std::vector<VkImageView> ImageViews() const noexcept;

private:
  Context& context_;

  VkSwapchainKHR swapchain_{VK_NULL_HANDLE};

  VkFormat imageFormat_{};
  VkExtent2D extent_{};

  std::vector<VkImage> images_{};
  std::vector<VkImageView> imageViews_{};

  void create_();

  VkSurfaceFormatKHR chooseSurfaceFormat_(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
  VkPresentModeKHR choosePresentMode_(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
  VkExtent2D chooseExtent_(const VkSurfaceCapabilitiesKHR& capabilities) const;
};

} // namespace engine::graphics::vulkan
