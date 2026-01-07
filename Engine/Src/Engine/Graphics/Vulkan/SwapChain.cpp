#include "SwapChain.hpp"

#include "Context.hpp"
#include "Device.hpp"
#include "ImageUtil.hpp"
#include "Sync.hpp"
#include "RenderPassCache.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <print>

namespace engine::graphics::vulkan {

SwapChain::SwapChain(Context& context) : context_(context) {
  create_();
}

SwapChain::~SwapChain() {
  Cleanup();
}

void SwapChain::Cleanup() {
  auto vkDevice = context_.GetDevice().Logical();

  framebuffers_.clear();

  for (auto imageView : imageViews_) {
    vkDestroyImageView(vkDevice, imageView, nullptr);
  }

  vkDestroyImageView(vkDevice, depthImageView_, nullptr);
  vkDestroyImage(vkDevice, depthImage_, nullptr);
  vkFreeMemory(vkDevice, depthImageMemory_, nullptr);

  vkDestroySwapchainKHR(vkDevice, swapchain_, nullptr);
}

void SwapChain::Recreate() {
  auto& window = context_.Window();
  int width = 0, height = 0;
  glfwGetFramebufferSize(&window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(&window, &width, &height);
    glfwWaitEvents();
  }

  auto vkDevice = context_.GetDevice().Logical();
  vkDeviceWaitIdle(vkDevice);

  Cleanup();
  create_();
  context_.GetSync().RecreatePerImageSemaphores();
}
void SwapChain::CreateFramebuffers(RenderPass& renderPass) {
  renderPasses_.push_back(&renderPass);
  framebuffers_.emplace_back(context_, *this, renderPass);
}

FramebufferSet& SwapChain::Framebuffers(const size_t setID) noexcept {
  assert(setID < framebuffers_.size());
  return framebuffers_[setID];
}

VkSwapchainKHR SwapChain::Handle() const noexcept {
  return swapchain_;
}

VkFormat SwapChain::ImageFormat() const noexcept {
  return imageFormat_;
}

VkExtent2D SwapChain::Extent() const noexcept {
  return extent_;
}

std::vector<VkImage> SwapChain::Images() const noexcept {
  return images_;
}
std::vector<VkImageView> SwapChain::ImageViews() const noexcept {
  return imageViews_;
}
VkImageView SwapChain::DepthImageView() const noexcept {
  return depthImageView_;
}

void SwapChain::create_() {
  const auto& device = context_.GetDevice();
  const auto vkDevice = device.Logical();
  const auto surface = context_.Surface();

  SwapChainSupportDetails swapChainSupport = device.QuerySwapChainSupport();

  VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat_(swapChainSupport.formats);
  VkPresentModeKHR presentMode = choosePresentMode_(swapChainSupport.presentModes);
  VkExtent2D extent = chooseExtent_(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = device.FindQueueFamilies();
  std::array queueFamilyIndices = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};
  if (indices.GraphicsFamily != indices.PresentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;     // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional
  }
  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(vkDevice, &createInfo, nullptr, &swapchain_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }

  vkGetSwapchainImagesKHR(vkDevice, swapchain_, &imageCount, nullptr);
  images_.resize(imageCount);
  vkGetSwapchainImagesKHR(vkDevice, swapchain_, &imageCount, images_.data());

  imageFormat_ = surfaceFormat.format;
  extent_ = extent;

  // Create image views
  imageViews_.resize(images_.size());
  for (uint32_t i = 0; i < images_.size(); i++) {
    if (auto result = imageutil::CreateImageView(device, images_[i], imageFormat_, VK_IMAGE_ASPECT_COLOR_BIT); result)
      imageViews_[i] = *result;
    else
      throw std::runtime_error("failed to create image views!");
  }

  for (const auto& renderPass : renderPasses_) {
    framebuffers_.emplace_back(context_, *this, *renderPass);
  }

  createDepthResources_();
}

VkSurfaceFormatKHR SwapChain::chooseSurfaceFormat_(const std::vector<VkSurfaceFormatKHR>& availableFormats) const {
  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR SwapChain::choosePresentMode_(const std::vector<VkPresentModeKHR>& availablePresentModes) const {
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::chooseExtent_(const VkSurfaceCapabilitiesKHR& capabilities) const {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }
  int width, height;
  glfwGetFramebufferSize(&context_.Window(), &width, &height);

  VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

  actualExtent.width =
      std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
  actualExtent.height =
      std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

  return actualExtent;
}

void SwapChain::createDepthResources_() {
  const auto& device = context_.GetDevice();
  VkFormat depthFormat = findDepthFormat_();

  imageutil::CreateImage(device,
                         this->extent_.width,
                         this->extent_.height,
                         depthFormat,
                         VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         depthImage_,
                         depthImageMemory_);

  auto result = imageutil::CreateImageView(device, depthImage_, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
  if (!result)
    throw std::runtime_error("failed to create depth image views!");
  depthImageView_ = *result;
}

VkFormat SwapChain::findDepthFormat_() {
  return context_.GetDevice().FindSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool SwapChain::hasStencilComponent_(const VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
} // namespace engine::graphics::vulkan