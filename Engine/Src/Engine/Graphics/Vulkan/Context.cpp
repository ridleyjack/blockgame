#include "Context.hpp"

#include "Command.hpp"
#include "Device.hpp"
#include "SwapChain.hpp"
#include "Sync.hpp"
#include "Config.hpp"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace engine::graphics::vulkan {
namespace debug {
VkResult CreateUtilsMessengerEXT(const VkInstance instance,
                                 const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                 const VkAllocationCallbacks* pAllocator,
                                 VkDebugUtilsMessengerEXT* pDebugMessenger) {
  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyUtilsMessengerEXT(VkInstance instance,
                              VkDebugUtilsMessengerEXT debugMessenger,
                              const VkAllocationCallbacks* pAllocator) {
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                             void* pUserData) {

  std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';

  return VK_FALSE;
}

void PopulateMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
}
} // namespace debug

namespace {
bool CheckValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char* layerName : config::ValidationLayers) {
    bool layerFound = false;
    for (const auto& layerProperties : availableLayers) {
      if (std::strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound) {
      return false;
    }
  }
  return true;
}

std::vector<const char*> GetRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
  if (config::EnableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  return extensions;
}
} // namespace

Context::Context(GLFWwindow* window) : window_(window) {
  createInstance_();
  setupDebugMessenger_();
  createSurface_();

  device_ = std::make_unique<Device>(*this);
  swapchain_ = std::make_unique<SwapChain>(*this);
  command_ = std::make_unique<Command>(*this);
  sync_ = std::make_unique<Sync>(*this);
}

Context::~Context() {
  if (device_)
    vkDeviceWaitIdle(device_->Logical());

  sync_.reset();
  command_.reset();
  swapchain_.reset();
  device_.reset();

  vkDestroySurfaceKHR(instance_, surface_, nullptr);

  if constexpr (config::EnableValidationLayers) {
    debug::DestroyUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
  }

  vkDestroyInstance(instance_, nullptr);
}

GLFWwindow& Context::Window() const noexcept {
  return *window_;
}

VkInstance Context::Instance() const noexcept {
  return instance_;
}

VkSurfaceKHR Context::Surface() const noexcept {
  return surface_;
}

Device& Context::GetDevice() const noexcept {
  return *device_;
}

SwapChain& Context::GetSwapchain() const noexcept {
  return *swapchain_;
}

Command& Context::GetCommand() const noexcept {
  return *command_;
}
Sync& Context::GetSync() const noexcept {
  return *sync_;
}

bool Context::GetFramebufferHasResized() const noexcept {
  return framebufferHasResized_;
}

void Context::SetFramebufferHasResized(bool value) noexcept {
  framebufferHasResized_ = value;
}

bool Context::WindowShouldClose() const noexcept {
  return glfwWindowShouldClose(window_);
}

void Context::WaitUntilIdle() const noexcept {
  vkDeviceWaitIdle(device_->Logical());
}

void Context::createInstance_() {
  if (config::EnableValidationLayers && !CheckValidationLayerSupport()) {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Vulkan Renderer";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledLayerCount = 0;
  auto extensions = GetRequiredExtensions();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if constexpr (config::EnableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(config::ValidationLayers.size());
    createInfo.ppEnabledLayerNames = config::ValidationLayers.data();

    debug::PopulateMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = &debugCreateInfo;
  }

  if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create vulkan instance!");
  }
}

void Context::setupDebugMessenger_() {
  if constexpr (config::EnableValidationLayers) {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    debug::PopulateMessengerCreateInfo(createInfo);
    if (debug::CreateUtilsMessengerEXT(instance_, &createInfo, nullptr, &debugMessenger_) != VK_SUCCESS) {
      throw std::runtime_error("failed to set up debug messenger!");
    }
  }
}

void Context::createSurface_() {
  if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}
} // namespace engine::graphics::vulkan