#include "VulkanContext.hpp"

#include "VulkanCommand.hpp"
#include "VulkanDevice.hpp"
#include "VulkanPipelineCache.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanSync.hpp"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace engine::gfx::vulkan {
namespace debug {
VkResult CreateUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                 const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
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

// ==============================
// Public Methods
// ==============================

Context::Context(GLFWwindow* window) {
  device_ = std::make_unique<Device>(*this);
  swapchain_ = std::make_unique<SwapChain>(*this);
  pipelineLibrary_ = std::make_unique<PipelineCache>(*this);
  command_ = std::make_unique<Command>(*this);
  sync_ = std::make_unique<Sync>(*this);
  window_ = window;
}

Context::~Context() {
  sync_.reset();
  command_.reset();
  swapchain_.reset();
  pipelineLibrary_.reset();
  device_.reset();

  vkDestroySurfaceKHR(instance_, surface_, nullptr);

  if constexpr (config::EnableValidationLayers) {
    debug::DestroyUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
  }

  vkDestroyInstance(instance_, nullptr);
  glfwDestroyWindow(window_);
  glfwTerminate();
}

void Context::Init() {
  createInstance_();
  setupDebugMessenger_();
  createSurface_();

  device_->Init();
  swapchain_->Init();
  command_->Init();
  sync_->Init();
}

GLFWwindow& Context::Window() const {
  return *window_;
}

VkInstance Context::Instance() const {
  return instance_;
}

VkSurfaceKHR Context::Surface() const {
  return surface_;
}

Device& Context::GetDevice() const {
  return *device_;
}

SwapChain& Context::GetSwapchain() const {
  return *swapchain_;
}

PipelineCache& Context::GetPipelineLibrary() const {
  return *pipelineLibrary_;
}

Command& Context::GetCommand() const {
  return *command_;
}
Sync& Context::GetSync() const {
  return *sync_;
}

bool Context::GetFramebufferResized() const {
  return framebufferResized_;
}

void Context::SetFramebufferResized(bool value) {
  framebufferResized_ = value;
}

bool Context::WindowShouldClose() const {
  return glfwWindowShouldClose(window_);
}

void Context::WaitUntilIdle() const {
  vkDeviceWaitIdle(device_->Logical());
}

// ==============================
// Private Methods
// ==============================

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
} // namespace engine::gfx::vulkan