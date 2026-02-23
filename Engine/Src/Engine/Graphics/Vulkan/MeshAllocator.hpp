#pragma once

#include "MeshGPU.hpp"
#include "StagingBuffer.hpp"

#include "Engine/Containers/HandlePool.hpp"
#include "Engine/Graphics/Handles.hpp"

namespace engine::graphics {
struct Mesh;
}

namespace engine::graphics::vulkan {
class Context;
class MeshBuffer;

struct AllocatedBuffer;

class MeshAllocator {
public:
  explicit MeshAllocator(Context& context, MeshBuffer& meshBuffer, StagingBuffer& stagingBuffer);
  ~MeshAllocator();

  MeshAllocator(const MeshAllocator&) = delete;
  MeshAllocator& operator=(const MeshAllocator&) = delete;

  uint32_t Create(const Mesh& mesh);
  void Delete(uint32_t meshID) noexcept;

  MeshGPU& Get(uint32_t meshID) noexcept;

private:
  Context& context_;
  MeshBuffer& meshBuffer_;
  StagingBuffer& stagingBuffer_;
  containers::HandlePool<MeshGPU> meshes_{};

  VkDeviceSize createVertexBuffer_(const Mesh& mesh);
  VkDeviceSize createIndexBuffer_(const Mesh& mesh);
};

} // namespace engine::graphics::vulkan