#pragma once

#include "Config.hpp"
#include "Device.hpp"

#include <glm/glm.hpp>

#include <array>

namespace engine::graphics::vulkan {

struct UniformBuffer {
  std::array<AllocatedBuffer, config::MaxFramesInFlight> Buffers{};
  std::array<void*, config::MaxFramesInFlight> MappedMemory{};
};

struct GlobalUBO {
  glm::mat4 View{1.0f};
  glm::mat4 Projection{1.0f};
};
} // namespace engine::graphics::vulkan
