#pragma once
#include "Device.hpp"

namespace engine::graphics::vulkan {

struct UniformBuffer {
  std::array<AllocatedBuffer, config::MaxFramesInFlight> Buffers{};
  std::array<void*, config::MaxFramesInFlight> MappedMemory{};
};

} // namespace engine::graphics::vulkan
