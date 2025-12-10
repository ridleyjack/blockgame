#pragma once

#include "Device.hpp"

namespace engine::graphics::vulkan {

struct MeshGPU {
  uint32_t VertexCount{};
  uint32_t IndexCount{};

  AllocatedBuffer VertexBuffer{};
  AllocatedBuffer IndexBuffer{};
};

} // namespace engine::graphics::vulkan
