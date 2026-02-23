#pragma once

#include <vulkan/vulkan.h>

namespace engine::graphics::vulkan {

struct MeshGPU {
  uint32_t VertexCount{};
  uint32_t IndexCount{};

  VkDeviceSize VertexOffset{};
  VkDeviceSize IndexOffset{};
};

} // namespace engine::graphics::vulkan
