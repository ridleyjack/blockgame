#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace engine::gfx::vulkan {

class Context;

class SwapChain {

public:
  explicit SwapChain(Context& context);
  ~SwapChain();

  SwapChain(const SwapChain&) = delete;
  SwapChain& operator=(const SwapChain&) = delete;

  void Init();
  void Cleanup();
  void Recreate();

  void CreateFrameBuffers(VkRenderPass renderPass);

  VkSwapchainKHR Handle() const;
  VkFormat ImageFormat() const;
  VkExtent2D Extent() const;
  VkFramebuffer Framebuffer(size_t index) const;

  std::vector<VkImage> Images() const;

private:
  Context& context_;

  VkSwapchainKHR swapchain_{VK_NULL_HANDLE};

  VkFormat imageFormat_{};
  VkExtent2D extent_{};

  std::vector<VkImage> images_{};
  std::vector<VkImageView> imageViews_{};

  std::vector<VkFramebuffer> framebuffers_{};

  VkSurfaceFormatKHR chooseSurfaceFormat_(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
  VkPresentModeKHR choosePresentMode_(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
  VkExtent2D chooseExtent_(const VkSurfaceCapabilitiesKHR& capabilities) const;
};

} // namespace Engine::GFX::Vulkan
