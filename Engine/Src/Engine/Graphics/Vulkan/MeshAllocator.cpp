#include "MeshAllocator.hpp"

#include "MeshBuffer.hpp"
#include "StagingBuffer.hpp"
#include "Uploader.hpp"

#include "Engine/Graphics/Mesh.hpp"

#include <stdexcept>

namespace engine::graphics::vulkan {

MeshAllocator::MeshAllocator(Context& context, Uploader& uploader, MeshBuffer& meshBuffer, StagingBuffer& stagingBuffer)
    : context_(context), uploader_(uploader), meshBuffer_(meshBuffer), stagingBuffer_(stagingBuffer) {}

MeshAllocator::~MeshAllocator() {
  for (size_t i = 0; i < meshes_.Size(); i++) {
    if (meshes_.Contains(i))
      deleteMesh(i);
  }
}

std::uint32_t MeshAllocator::Create(const Mesh& mesh) {
  MeshGPU meshGPU{
      .VertexCount = static_cast<uint32_t>(mesh.Vertices.size()),
      .IndexCount = static_cast<uint32_t>(mesh.Indices.size()),
  };
  auto [cmd, batchID] = uploader_.GetCurrent();

  meshGPU.VertexOffset = uploadToMeshBuffer_(std::as_bytes(std::span(mesh.Vertices)), alignof(Vertex), cmd, batchID);

  meshGPU.IndexOffset =
      uploadToMeshBuffer_(std::as_bytes(std::span(mesh.Indices)), alignof(std::uint32_t), cmd, batchID);

  std::uint32_t meshID = meshes_.Create(meshGPU);

  Uploader::UploadRequest request{.OnComplete = [this, meshID]() noexcept {
    auto& gpuMesh = meshes_.Get(meshID);
    if (gpuMesh.State == MeshState::Deleting)
      deleteMesh(meshID);
    else
      gpuMesh.State = MeshState::Ready;
  }};

  uploader_.Queue(std::move(request));

  return meshID;
};

void MeshAllocator::DeleteDeferred(const std::uint32_t meshID, const std::uint32_t retireFrame) {
  assert(meshes_.Contains(meshID));
  auto& mesh = meshes_.Get(meshID);

  if (mesh.State == MeshState::Deleting)
    return;
  if (mesh.State == MeshState::Ready)
    pendingDeletes_.push_back({.MeshID = meshID, .RetireFrame = retireFrame});

  mesh.State = MeshState::Deleting;
}

void MeshAllocator::ProcessDeferredDeletions(const std::uint32_t currentframe) {
  std::erase_if(pendingDeletes_, [&](const PendingDelete& pending) {
    if (pending.RetireFrame != currentframe)
      return false;

    deleteMesh(pending.MeshID);
    return true;
  });
}

MeshAllocator::MeshGPU& MeshAllocator::Get(const std::uint32_t meshID) noexcept {
  assert(meshes_.Contains(meshID));
  return meshes_.Get(meshID);
}

VkDeviceSize MeshAllocator::uploadToMeshBuffer_(std::span<const std::byte> data,
                                                const VkDeviceSize alignment,
                                                VkCommandBuffer cmd,
                                                const std::uint64_t batchID) const {
  const VkDeviceSize size = data.size();
  // Staging buffer.
  const auto stagingOffset = stagingBuffer_.Write(data, alignment, batchID);
  if (!stagingOffset)
    throw std::runtime_error("MeshAllocator failed to write to staging buffer");

  // Mesh buffer.
  const auto meshOffset = meshBuffer_.Allocate(size);
  if (!meshOffset)
    throw std::runtime_error("MeshAllocator failed to write to mesh buffer");

  // Record the copy.
  const VkBufferCopy copyRegion{.srcOffset = *stagingOffset, .dstOffset = *meshOffset, .size = size};
  vkCmdCopyBuffer(cmd, stagingBuffer_.Handle(), meshBuffer_.Handle(), 1, &copyRegion);

  return *meshOffset;
}

void MeshAllocator::deleteMesh(const std::uint32_t meshID) {
  const auto& mesh = meshes_.Get(meshID);
  meshBuffer_.Free(mesh.VertexOffset);
  meshBuffer_.Free(mesh.IndexOffset);
  meshes_.Delete(meshID);
}

} // namespace engine::graphics::vulkan
