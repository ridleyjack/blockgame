#include "MeshAllocator.hpp"

#include "Command.hpp"
#include "Context.hpp"
#include "Device.hpp"
#include "MeshGPU.hpp"
#include "MeshBuffer.hpp"

#include "Engine/Graphics/Mesh.hpp"

#include <cstring>

namespace engine::graphics::vulkan {

MeshAllocator::MeshAllocator(Context& context, MeshBuffer& meshBuffer, StagingBuffer& stagingBuffer)
    : context_(context), meshBuffer_(meshBuffer), stagingBuffer_(stagingBuffer) {}

MeshAllocator::~MeshAllocator() {
  for (size_t i = 0; i < meshes_.Size(); i++) {
    if (meshes_.Contains(i))
      this->Delete(i);
  }
}

uint32_t MeshAllocator::Create(const Mesh& mesh) {
  const VkDeviceSize vertexOffset = createVertexBuffer_(mesh);
  const VkDeviceSize indexOffset = createIndexBuffer_(mesh);

  MeshGPU gpuMesh{.VertexCount = static_cast<uint32_t>(mesh.Vertices.size()),
                  .IndexCount = static_cast<uint32_t>(mesh.Indices.size()),
                  .VertexOffset = vertexOffset,
                  .IndexOffset = indexOffset};

  const uint32_t meshID = meshes_.Create(gpuMesh);
  return meshID;
};

void MeshAllocator::Delete(const uint32_t meshID) noexcept {
  const auto& device = context_.GetDevice();
  const MeshGPU& mesh = meshes_.Get(meshID);
  // Todo: Free the mesh when supported
  meshes_.Delete(meshID);
}

MeshGPU& MeshAllocator::Get(const uint32_t meshID) noexcept {
  assert(meshes_.Contains(meshID));
  return meshes_.Get(meshID);
}

VkDeviceSize MeshAllocator::createVertexBuffer_(const Mesh& mesh) {
  const auto& device = context_.GetDevice();
  auto vkDevice = device.Logical();
  const auto& cmd = context_.GetCommand();

  const VkDeviceSize bufferSize = sizeof(mesh.Vertices[0]) * mesh.Vertices.size();
  // Allocate Staging Buffer.
  VkDeviceSize stagingOffset =
      stagingBuffer_.Allocate(bufferSize, std::max<VkDeviceSize>(alignof(Vertex), alignof(std::uint32_t)));

  // Fill Staging Buffer
  std::memcpy(static_cast<std::byte*>(stagingBuffer_.Mapping()) + stagingOffset, mesh.Vertices.data(), bufferSize);

  // Allocate Mesh Buffer.
  VkDeviceSize meshOffset = meshBuffer_.AllocateDeviceLocal(bufferSize);

  // Staging Buffer to Vertex Buffer.
  VkCommandBuffer commandBuffer = cmd.BeginSingleTimeCommands();
  VkBufferCopy copyRegion{.srcOffset = stagingOffset, .dstOffset = meshOffset, .size = bufferSize};
  vkCmdCopyBuffer(commandBuffer, stagingBuffer_.Handle(), meshBuffer_.Handle(), 1, &copyRegion);
  cmd.EndSingleTimeCommands(commandBuffer);

  return meshOffset;
}

VkDeviceSize MeshAllocator::createIndexBuffer_(const Mesh& mesh) {
  const auto& device = context_.GetDevice();
  const auto& cmd = context_.GetCommand();

  const VkDeviceSize bufferSize = sizeof(mesh.Indices[0]) * mesh.Indices.size();
  // Create Staging Buffer.
  VkDeviceSize stagingOffset =
      stagingBuffer_.Allocate(bufferSize, std::max<VkDeviceSize>(alignof(Vertex), alignof(std::uint32_t)));

  // Fill Staging Buffer
  std::memcpy(static_cast<std::byte*>(stagingBuffer_.Mapping()) + stagingOffset, mesh.Indices.data(), bufferSize);

  // Allocate index buffer.
  VkDeviceSize indexOffset = meshBuffer_.AllocateDeviceLocal(bufferSize);

  // Staging Buffer to Vertex Buffer.
  VkCommandBuffer commandBuffer = cmd.BeginSingleTimeCommands();
  VkBufferCopy copyRegion{.srcOffset = stagingOffset, .dstOffset = indexOffset, .size = bufferSize};
  vkCmdCopyBuffer(commandBuffer, stagingBuffer_.Handle(), meshBuffer_.Handle(), 1, &copyRegion);
  cmd.EndSingleTimeCommands(commandBuffer);

  return indexOffset;
}
} // namespace engine::graphics::vulkan