#pragma once

#include "FramebufferSet.hpp"

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

  void CreateFramebuffers(RenderPass& renderPass);
  FramebufferSet& Framebuffers(size_t setID) noexcept;

  VkSwapchainKHR Handle() const noexcept;
  VkFormat ImageFormat() const noexcept;
  VkExtent2D Extent() const noexcept;

  std::vector<VkImage> Images() const noexcept;
  std::vector<VkImageView> ImageViews() const noexcept;

  VkImageView DepthImageView() const noexcept;

private:
  Context& context_;

  std::vector<RenderPass*> renderPasses_{};
  VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
  VkFormat imageFormat_{};
  VkExtent2D extent_{};

  std::vector<VkImage> images_{};
  std::vector<VkImageView> imageViews_{};
  std::vector<FramebufferSet> framebuffers_{};

  void create_();

  VkSurfaceFormatKHR chooseSurfaceFormat_(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
  VkPresentModeKHR choosePresentMode_(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
  VkExtent2D chooseExtent_(const VkSurfaceCapabilitiesKHR& capabilities) const;

  VkImage depthImage_{VK_NULL_HANDLE};
  VkDeviceMemory depthImageMemory_{VK_NULL_HANDLE};
  VkImageView depthImageView_{VK_NULL_HANDLE};

  void createDepthResources_();
  VkFormat findDepthFormat_();
  bool hasStencilComponent_(VkFormat format);
};
} // namespace engine::graphics::vulkan
