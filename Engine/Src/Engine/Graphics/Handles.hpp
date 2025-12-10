#pragma once
#include <cstdint>

namespace engine::graphics {
struct PipelineHandle {
  uint32_t PipelineID{};
};

struct MeshHandle {
  uint32_t MeshID{};
};

struct UniformHandle {
  uint32_t UniformID{};
};
} // namespace engine::graphics