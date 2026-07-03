#pragma once
#include <cstdint>
#include <type_traits>

namespace engine::graphics {
struct PipelineHandle {
  std::uint32_t PipelineID{};
};

struct MeshHandle {
  std::uint32_t MeshID{};
};

struct TextureHandle {
  std::uint32_t TextureID{};
};

struct MaterialHandle {
  std::uint32_t TextureID{};
  std::uint32_t DescriptorSetID{};
};

template <class T>
concept ShaderDataStruct = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

template <class T> struct ShaderDataHandle {
  std::uint32_t ShaderDataID{};
};
} // namespace engine::graphics