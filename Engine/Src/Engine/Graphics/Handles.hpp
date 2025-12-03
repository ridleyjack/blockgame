#pragma once
#include <cstdint>

namespace engine::graphics {
struct PipelineHandle {
  uint32_t PipelineID{};
};

struct MeshHandle {
  uint32_t MeshID{};
};
} // namespace engine::graphics