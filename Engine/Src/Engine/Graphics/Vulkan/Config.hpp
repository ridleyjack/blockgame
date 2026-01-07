#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace engine::graphics::vulkan::config {

constexpr bool EnableValidationLayers{
#ifdef NDEBUG
    false
#else
    true
#endif
};

const std::vector<const char*> ValidationLayers{"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> DeviceExtensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

constexpr uint32_t Width = 800;
constexpr uint32_t Height = 600;
constexpr int MaxFramesInFlight = 2;

} // namespace engine::graphics::vulkan::config