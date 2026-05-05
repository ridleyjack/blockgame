#pragma once

#include "Engine/Containers/HandlePool.hpp"

#include <vulkan/vulkan.h>

#include <expected>
#include <span>
#include <string_view>
#include <vector>

namespace engine::graphics {
struct Mesh;
}

namespace engine::graphics::vulkan {
class Context;
class Uploader;

class StagingBuffer;
class MeshBuffer;

enum class MeshError : std::uint8_t {
  OutOfStagingMemory,
  OutOfMeshMemory
};

constexpr std::string_view ToString(const MeshError error) noexcept {
  switch (error) {
  case MeshError::OutOfStagingMemory:
    return "OutOfStagingMemory";

  case MeshError::OutOfMeshMemory:
    return "OutOfMeshMemory";
  }
  return "UnknownMeshError";
}

enum class MeshState : std::uint8_t {
  Uploading,
  Ready,
  Deleting
};

class MeshAllocator {
public:
  struct MeshGPU {
    uint32_t VertexCount{};
    uint32_t IndexCount{};

    VkDeviceSize VertexOffset{};
    VkDeviceSize IndexOffset{};

    MeshState State{MeshState::Uploading};
  };

  explicit MeshAllocator(Context& context, Uploader& uploader, MeshBuffer& meshBuffer, StagingBuffer& stagingBuffer);
  ~MeshAllocator();

  MeshAllocator(const MeshAllocator&) = delete;
  MeshAllocator& operator=(const MeshAllocator&) = delete;

  std::expected<uint32_t, MeshError> Create(const Mesh& mesh);

  void DeleteDeferred(std::uint32_t meshID, std::uint32_t retireFrame);
  void ProcessDeferredDeletions(std::uint32_t currentframe);

  MeshGPU& Get(std::uint32_t meshID) noexcept;

private:
  struct PendingDelete {
    std::uint32_t MeshID{};
    std::uint32_t RetireFrame{};
  };

  Context& context_;
  Uploader& uploader_;

  MeshBuffer& meshBuffer_;
  StagingBuffer& stagingBuffer_;
  containers::HandlePool<MeshGPU> meshes_{};

  std::vector<PendingDelete> pendingDeletes_{};

  std::expected<VkDeviceSize, MeshError> uploadToMeshBuffer_(std::span<const std::byte> data,
                                                             VkDeviceSize alignment,
                                                             VkCommandBuffer cmd,
                                                             std::uint64_t batchID) const;
  void deleteMesh(std::uint32_t meshID);
};

} // namespace engine::graphics::vulkan