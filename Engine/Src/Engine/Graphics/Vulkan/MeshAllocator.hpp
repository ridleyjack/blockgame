#pragma once

#include "MeshGPU.hpp"

#include "Engine/Containers/HandlePool.hpp"
#include "Engine/Graphics/Handles.hpp"

namespace engine::graphics {
struct Mesh;
}

namespace engine::graphics::vulkan {
class Context;

struct AllocatedBuffer;

class MeshAllocator {
public:
  explicit MeshAllocator(Context& context);
  ~MeshAllocator();

  MeshAllocator(const MeshAllocator&) = delete;
  MeshAllocator& operator=(const MeshAllocator&) = delete;

  uint32_t Create(const Mesh& mesh);
  void Delete(uint32_t meshID) noexcept;

  MeshGPU& Get(uint32_t meshID) noexcept;

private:
  Context& context_;

  containers::HandlePool<MeshGPU> meshes_{};

  AllocatedBuffer createVertexBuffer_(const Mesh& mesh) const;
  AllocatedBuffer createIndexBuffer_(const Mesh& mesh) const;
};

} // namespace engine::graphics::vulkan