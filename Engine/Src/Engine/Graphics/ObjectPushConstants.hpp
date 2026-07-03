#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <array>

namespace engine::graphics {

struct ObjectPushConstants {
  static constexpr std::uint32_t MaxShaderDataSlots = 8;
  
  VkDeviceAddress CameraAddress{};
  std::array<VkDeviceAddress, MaxShaderDataSlots> ShaderData{};
};

} // namespace engine::graphics
