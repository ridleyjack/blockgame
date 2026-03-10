#include "MeshAllocator.hpp"

#include "Command.hpp"
#include "Context.hpp"
#include "MeshGPU.hpp"
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

uint32_t MeshAllocator::Create(const Mesh& mesh) {

  MeshGPU gpuMesh{
      .VertexCount = static_cast<uint32_t>(mesh.Vertices.size()),
      .IndexCount = static_cast<uint32_t>(mesh.Indices.size()),
  };
  const uint32_t meshID = meshes_.Create(gpuMesh);

  Uploader::UploadRequest request{};
  request.Record = [this, &mesh, meshID](VkCommandBuffer cmd, std::uint64_t batchID) noexcept {
    const auto vertexOffset =
        uploadToMeshBuffer_(std::as_bytes(std::span(mesh.Vertices)), alignof(Vertex), cmd, batchID);
    const auto indexOffset =
        uploadToMeshBuffer_(std::as_bytes(std::span(mesh.Indices)), alignof(std::uint32_t), cmd, batchID);

    auto& gpuMesh = meshes_.Get(meshID);
    gpuMesh.VertexOffset = vertexOffset;
    gpuMesh.IndexOffset = indexOffset;
  };
  request.OnComplete = [this, meshID]() noexcept {
    auto& gpuMesh = meshes_.Get(meshID);
    gpuMesh.Ready = true;
  };

  uploader_.Queue(std::move(request));

  return meshID;
};

void MeshAllocator::Delete(const std::uint32_t meshID) {
  const auto& mesh = meshes_.Get(meshID);
  assert(mesh.Ready); // TODO: Can Delete meshes pending upload.
  meshBuffer_.Free(mesh.VertexOffset);
  meshBuffer_.Free(mesh.IndexOffset);
  meshes_.Delete(meshID);
}

MeshGPU& MeshAllocator::Get(const std::uint32_t meshID) noexcept {
  assert(meshes_.Contains(meshID));
  return meshes_.Get(meshID);
}

VkDeviceSize MeshAllocator::uploadToMeshBuffer_(std::span<const std::byte> data,
                                                const VkDeviceSize alignment,
                                                VkCommandBuffer cmd,
                                                const std::uint64_t batchID) const {
  const VkDeviceSize size = data.size();

  // Staging buffer.
  const VkDeviceSize stagingOffset = stagingBuffer_.Write(data, alignment, batchID);

  // Mesh buffer.
  const VkDeviceSize meshOffset = meshBuffer_.Allocate(size);
  const VkBufferCopy copyRegion{.srcOffset = stagingOffset, .dstOffset = meshOffset, .size = size};
  vkCmdCopyBuffer(cmd, stagingBuffer_.Handle(), meshBuffer_.Handle(), 1, &copyRegion);

  return meshOffset;
}
} // namespace engine::graphics::vulkan