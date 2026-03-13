#include "MeshAllocator.hpp"

#include "Command.hpp"
#include "Context.hpp"
#include "MeshBuffer.hpp"
#include "StagingBuffer.hpp"
#include "Uploader.hpp"

#include "Engine/Graphics/Mesh.hpp"

#include <cstring>

namespace engine::graphics::vulkan {

MeshAllocator::MeshAllocator(Context& context, Uploader& uploader, MeshBuffer& meshBuffer, StagingBuffer& stagingBuffer)
    : context_(context), uploader_(uploader), meshBuffer_(meshBuffer), stagingBuffer_(stagingBuffer) {}

MeshAllocator::~MeshAllocator() {
  for (size_t i = 0; i < meshes_.Size(); i++) {
    if (meshes_.Contains(i))
      this->Delete(i);
  }
}

std::expected<uint32_t, MeshError> MeshAllocator::Create(const Mesh& mesh) {
  MeshGPU meshGPU{
      .VertexCount = static_cast<uint32_t>(mesh.Vertices.size()),
      .IndexCount = static_cast<uint32_t>(mesh.Indices.size()),
  };
  auto [cmd, batchID] = uploader_.GetCurrent();
  // TODO: If mesh creation encounters an error we still record copy commands on now freed buffers. We need to cancel
  // the whole command or not free the buffers until execution is complete.

  // Upload Vertices.
  if (const std::expected<VkDeviceSize, MeshError> result =
          uploadToMeshBuffer_(std::as_bytes(std::span(mesh.Vertices)), alignof(Vertex), cmd, batchID);
      !result) {
    return std::unexpected(result.error());
  } else {
    meshGPU.VertexOffset = *result;
  }
  // Upload Indices.
  if (const std::expected<VkDeviceSize, MeshError> result =
          uploadToMeshBuffer_(std::as_bytes(std::span(mesh.Indices)), alignof(std::uint32_t), cmd, batchID);
      !result) {
    meshBuffer_.Free(meshGPU.VertexOffset);
    return std::unexpected(result.error());
  } else {
    meshGPU.IndexOffset = *result;
  }

  std::uint32_t meshID = meshes_.Create(meshGPU);

  Uploader::UploadRequest request{.OnComplete = [this, meshID]() noexcept {
    auto& gpuMesh = meshes_.Get(meshID);
    if (gpuMesh.State == MeshState::Deleting)
      Delete(meshID);
    else
      gpuMesh.State = MeshState::Ready;
  }};

  uploader_.Queue(std::move(request));

  return meshID;
};

void MeshAllocator::Delete(const std::uint32_t meshID) {
  assert(meshes_.Contains(meshID));
  auto& mesh = meshes_.Get(meshID);
  if (mesh.State == MeshState::Uploading) {
    mesh.State = MeshState::Deleting;
    return;
  }
  meshBuffer_.Free(mesh.VertexOffset);
  meshBuffer_.Free(mesh.IndexOffset);
  meshes_.Delete(meshID);
}

MeshAllocator::MeshGPU& MeshAllocator::Get(const std::uint32_t meshID) noexcept {
  assert(meshes_.Contains(meshID));
  return meshes_.Get(meshID);
}

std::expected<VkDeviceSize, MeshError> MeshAllocator::uploadToMeshBuffer_(std::span<const std::byte> data,
                                                                          const VkDeviceSize alignment,
                                                                          VkCommandBuffer cmd,
                                                                          const std::uint64_t batchID) const {
  const VkDeviceSize size = data.size();
  // Staging buffer.
  const VkDeviceSize stagingOffset = stagingBuffer_.Write(data, alignment, batchID);
  if (stagingOffset == std::numeric_limits<VkDeviceSize>::max())
    return std::unexpected(MeshError::OutOfStagingMemory);

  // Mesh buffer.
  const VkDeviceSize meshOffset = meshBuffer_.Allocate(size);
  if (meshOffset == std::numeric_limits<VkDeviceSize>::max())
    return std::unexpected(MeshError::OutOfMeshMemory);

  // Record the copy.
  const VkBufferCopy copyRegion{.srcOffset = stagingOffset, .dstOffset = meshOffset, .size = size};
  vkCmdCopyBuffer(cmd, stagingBuffer_.Handle(), meshBuffer_.Handle(), 1, &copyRegion);

  return meshOffset;
}
} // namespace engine::graphics::vulkan