#pragma once
#include "Handles.hpp"

#include <span>
#include <typeindex>

namespace engine::graphics {

struct ShaderDataBinding {
  std::uint32_t ShaderDataID{};
  std::size_t Size{};
  std::type_index Type{typeid(void)};
};

template <class T> ShaderDataBinding BindShaderData(ShaderDataHandle<T> handle) {
  return ShaderDataBinding{
      .ShaderDataID = handle.ShaderDataID,
      .Size = sizeof(T),
      .Type = std::type_index(typeid(T)),
  };
}

struct SubmitInfo {
  PipelineHandle Pipeline{};
  MeshHandle Mesh{};
  MaterialHandle Material{};

  std::span<const ShaderDataBinding> ShaderData{};
};
} // namespace engine::graphics