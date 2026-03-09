#pragma once

#include "MeshGPU.hpp"

#include "Engine/Containers/HandlePool.hpp"
#include "Engine/Graphics/Handles.hpp"

#include <span>

namespace engine::graphics {
struct Mesh;
}

namespace engine::graphics::vulkan {
class Context;
class Uploader;

class StagingBuffer;
class MeshBuffer;

struct AllocatedBuffer;

class MeshAllocator {
public:
  explicit MeshAllocator(Context& context, Uploader& uploader, MeshBuffer& meshBuffer, StagingBuffer& stagingBuffer);
  ~MeshAllocator();

  MeshAllocator(const MeshAllocator&) = delete;
  MeshAllocator& operator=(const MeshAllocator&) = delete;

  uint32_t Create(const Mesh& mesh);
  void Delete(std::uint32_t meshID);

  MeshGPU& Get(std::uint32_t meshID) noexcept;

private:
  Context& context_;
  Uploader& uploader_;

  MeshBuffer& meshBuffer_;
  StagingBuffer& stagingBuffer_;
  containers::HandlePool<MeshGPU> meshes_{};

  VkDeviceSize uploadToMeshBuffer_(std::span<const std::byte> data,
                                   VkDeviceSize alignment,
                                   VkCommandBuffer cmd,
                                   std::uint64_t batchID) const;
};

} // namespace engine::graphics::vulkan