#include "VulkanDevice.hpp"

#include "VulkanContext.hpp"

#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

namespace engine::gfx::vulkan {

// ==============================
// Public Methods
// ==============================

Device::Device(Context& context) : context_{context} {}

Device::~Device() {
  vkDestroyDevice(device_, nullptr);
}

void Device::Init() {
  pickPhysicalDevice_();

  QueueFamilyIndices indices = findQueueFamilies_(physicalDevice_, context_.Surface());
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.pEnabledFeatures = &deviceFeatures;
  if (config::EnableValidationLayers) { // These values are ignored in modern Vulkan.
    createInfo.enabledLayerCount = static_cast<uint32_t>(config::ValidationLayers.size());
    createInfo.ppEnabledLayerNames = config::ValidationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }
  createInfo.enabledExtensionCount = static_cast<uint32_t>(config::DeviceExtensions.size());
  createInfo.ppEnabledExtensionNames = config::DeviceExtensions.data();

  if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }

  std::fprintf(stderr, "[Device] Logical=%p Physical=%p\n", (void*)device_, (void*)physicalDevice_);
  vkGetDeviceQueue(device_, indices.GraphicsFamily.value(), 0, &graphicsQueue_);
  vkGetDeviceQueue(device_, indices.PresentFamily.value(), 0, &presentQueue_);
}

VkDevice Device::Logical() const {
  return device_;
}
VkPhysicalDevice Device::Physical() const {
  return physicalDevice_;
}

VkQueue Device::GraphicsQueue() const {
  return graphicsQueue_;
}

VkQueue Device::PresentQueue() const {
  return presentQueue_;
}

QueueFamilyIndices Device::FindQueueFamilies() const {
  return findQueueFamilies_(physicalDevice_, context_.Surface());
}

SwapChainSupportDetails Device::QuerySwapChainSupport() const {
  auto surface = context_.Surface();

  SwapChainSupportDetails details{};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface, &presentModeCount, nullptr);
  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}
uint32_t Device::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if (typeFilter & (1 << i) && ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

VkImageView Device::CreateImageView(VkImage image, VkFormat format) const {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView imageView;
  if (vkCreateImageView(context_.GetDevice().Logical(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view!");
  }

  return imageView;
}

// ==============================
// Private Methods
// ==============================

bool Device::checkDeviceExtensionSupport_(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

  std::set<std::string> requiredExtensions(config::DeviceExtensions.begin(), config::DeviceExtensions.end());

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }
  return requiredExtensions.empty();
}

bool Device::isDeviceSuitable_(VkPhysicalDevice device, VkSurfaceKHR surface) {
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  std::cout << deviceProperties.deviceName << std::endl;

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  QueueFamilyIndices indices = findQueueFamilies_(device, surface);

  bool extensionsSupported = checkDeviceExtensionSupport_(device);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport_(device, surface);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices Device::findQueueFamilies_(VkPhysicalDevice device, VkSurfaceKHR surface) {
  QueueFamilyIndices indices{};

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  for (size_t i = 0; i < queueFamilies.size(); i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.GraphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (presentSupport) {
      indices.PresentFamily = i;
    }

    if (indices.IsComplete()) {
      break;
    }
  }

  return indices;
}

SwapChainSupportDetails Device::querySwapChainSupport_(VkPhysicalDevice device, VkSurfaceKHR surface) {
  SwapChainSupportDetails details{};

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

void Device::pickPhysicalDevice_() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(context_.Instance(), &deviceCount, nullptr);
  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(context_.Instance(), &deviceCount, devices.data());

  std::vector<VkPhysicalDevice> suitableDevices{};
  for (const auto& device : devices) {
    if (isDeviceSuitable_(device, context_.Surface())) {
      suitableDevices.push_back(device);
    }
  }

  if (!suitableDevices.empty()) {
    physicalDevice_ = suitableDevices[0];
  }

  if (physicalDevice_ == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

} // namespace engine::gfx::vulkan