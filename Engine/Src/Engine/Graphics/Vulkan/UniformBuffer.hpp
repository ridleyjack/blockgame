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

struct UniformBufferObject {
  alignas(16) glm::mat4 Model{};
  alignas(16) glm::mat4 View{};
  alignas(16) glm::mat4 Projection{};
};

} // namespace engine::graphics::vulkan
