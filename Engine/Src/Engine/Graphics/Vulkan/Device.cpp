#include "Device.hpp"

#include "CheckVk.hpp"
#include "Context.hpp"
#include "Command.hpp"
#include "Config.hpp"
#include "Engine/Fatal.hpp"

#include <set>
#include <vector>
#include <print>

namespace {

struct RequiredDeviceFeatures {
  VkPhysicalDeviceFeatures VK10{
      .samplerAnisotropy = VK_TRUE,
  };
  VkPhysicalDeviceVulkan12Features VK12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .bufferDeviceAddress = VK_TRUE,
  };
  VkPhysicalDeviceVulkan13Features VK13{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .synchronization2 = VK_TRUE,
      .dynamicRendering = VK_TRUE,
  };

  RequiredDeviceFeatures() {
    VK13.pNext = &VK12;
  }
  void* Chain() {
    return &VK13;
  }
};

struct SupportedDeviceFeatures {
  VkPhysicalDeviceFeatures2 vk10{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
  };
  VkPhysicalDeviceVulkan12Features vk12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
  };
  VkPhysicalDeviceVulkan13Features vk13{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
  };
  SupportedDeviceFeatures() {
    vk10.pNext = &vk12;
    vk12.pNext = &vk13;
  }
  bool SupportsRequired() const {
    return vk10.features.samplerAnisotropy && vk12.bufferDeviceAddress && vk13.synchronization2 &&
           vk13.dynamicRendering;
  }
};

} // namespace

namespace engine::graphics::vulkan {

Device::Device(Context& context) : context_{context} {
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

  RequiredDeviceFeatures requiredDeviceFeatures{};

  VkDeviceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = requiredDeviceFeatures.Chain(),
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledLayerCount = 0,
      .enabledExtensionCount = static_cast<uint32_t>(config::DeviceExtensions.size()),
      .ppEnabledExtensionNames = config::DeviceExtensions.data(),
      .pEnabledFeatures = &requiredDeviceFeatures.VK10,
  };
  if (config::EnableValidationLayers) { // These values are ignored in modern Vulkan.
    createInfo.enabledLayerCount = static_cast<uint32_t>(config::ValidationLayers.size());
    createInfo.ppEnabledLayerNames = config::ValidationLayers.data();
  }

  CheckVk(vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_), "vkCreateDevice");

  vkGetDeviceQueue(device_, indices.GraphicsFamily.value(), 0, &graphicsQueue_);
  vkGetDeviceQueue(device_, indices.PresentFamily.value(), 0, &presentQueue_);
}

Device::~Device() {
  vkDestroyDevice(device_, nullptr);
}

VkDevice Device::Logical() const noexcept {
  return device_;
}
VkPhysicalDevice Device::Physical() const noexcept {
  return physicalDevice_;
}

VkQueue Device::GraphicsQueue() const noexcept {
  return graphicsQueue_;
}

VkQueue Device::PresentQueue() const noexcept {
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

  Fatal("Failed to find suitable memory type!");
}
VkFormat Device::FindSupportedFormat(const std::vector<VkFormat>& candidates,
                                     const VkImageTiling tiling,
                                     const VkFormatFeatureFlags features) const {
  for (const VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
      return format;
    }
    if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  Fatal("Failed to find supported format!");
}
VkSampleCountFlagBits Device::MsaaSamples() const noexcept {
  return msaaSamples_;
}

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

  SupportedDeviceFeatures supportedFeatures{};
  vkGetPhysicalDeviceFeatures2(device, &supportedFeatures.vk10);

  QueueFamilyIndices indices = findQueueFamilies_(device, surface);

  bool extensionsSupported = checkDeviceExtensionSupport_(device);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport_(device, surface);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.SupportsRequired();
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
    Fatal("Failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(context_.Instance(), &deviceCount, devices.data());

  for (const auto& device : devices) {
    if (isDeviceSuitable_(device, context_.Surface())) {
      physicalDevice_ = device;
      msaaSamples_ = getMaxUsableSampleCount_();
      break;
    }
  }

  if (physicalDevice_ == VK_NULL_HANDLE) {
    Fatal("Failed to find a suitable GPU!");
  }
}

VkSampleCountFlagBits Device::getMaxUsableSampleCount_() const noexcept {
  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice_, &physicalDeviceProperties);

  const VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                    physicalDeviceProperties.limits.framebufferDepthSampleCounts;
  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    return VK_SAMPLE_COUNT_2_BIT;
  }
  return VK_SAMPLE_COUNT_1_BIT;
}

} // namespace engine::graphics::vulkan
