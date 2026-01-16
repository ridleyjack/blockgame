#pragma once
#include <cstdint>

namespace engine::graphics {
struct RenderPassHandle {
  uint32_t RenderPassID;
};

struct PipelineHandle {
  uint32_t PipelineID{};
};

struct MeshHandle {
  uint32_t MeshID{};
};

struct UniformHandle {
  uint32_t UniformID{};
};

struct TextureHandle {
  uint32_t TextureID{};
};

struct MaterialHandle {
  uint32_t TextureID{};
  uint32_t DescriptorSetID{};
};
} // namespace engine::graphics