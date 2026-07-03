#pragma once

#include <string>
#include <typeindex>
#include <vector>
#include <cstdint>

namespace engine::graphics {

struct ShaderDataType {
  std::size_t Size{};
  std::type_index Type{typeid(void)};
};

template <class T> ShaderDataType MakeShaderDataType() {
  return ShaderDataType{
      .Size = sizeof(T),
      .Type = typeid(T),
  };
}

enum class PipelineKind : std::uint8_t {
  SolidTexture,
  SolidGeometry
};

struct PipelineCreateInfo {
  PipelineKind Kind{};
  std::string VertexShaderFile{};
  std::string FragmentShaderFile{};

  std::vector<ShaderDataType> ShaderDataSlots{};
};
} // namespace engine::graphics