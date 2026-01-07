#include "MeshAllocator.hpp"

#include "Context.hpp"
#include "MeshGPU.hpp"
#include "Engine/Graphics/Mesh.hpp"

#include <cstring>

namespace engine::graphics::vulkan {

MeshAllocator::MeshAllocator(Context& context) : context_(context) {}

MeshAllocator::~MeshAllocator() {
  for (size_t i = 0; i < meshes_.Size(); i++) {
    if (meshes_.Contains(i))
      this->Delete(i);
  }
}

uint32_t MeshAllocator::Create(const Mesh& mesh) {
  const AllocatedBuffer gpuVertex = createVertexBuffer_(mesh);
  const AllocatedBuffer gpuIndex = createIndexBuffer_(mesh);

  MeshGPU gpuMesh{.VertexCount = static_cast<uint32_t>(mesh.Vertices.size()),
                  .IndexCount = static_cast<uint32_t>(mesh.Indices.size()),
                  .VertexBuffer = gpuVertex,
                  .IndexBuffer = gpuIndex};

  const uint32_t meshID = meshes_.Create(gpuMesh);
  return meshID;
};

void MeshAllocator::Delete(const uint32_t meshID) noexcept {
  const auto& device = context_.GetDevice();
  const MeshGPU& mesh = meshes_.Get(meshID);
  device.DestroyBuffer(mesh.VertexBuffer);
  device.DestroyBuffer(mesh.IndexBuffer);
  meshes_.Delete(meshID);
}

MeshGPU& MeshAllocator::Get(const uint32_t meshID) noexcept {
  assert(meshes_.Contains(meshID));
  return meshes_.Get(meshID);
}

AllocatedBuffer MeshAllocator::createVertexBuffer_(const Mesh& mesh) const {
  const auto& device = context_.GetDevice();
  const auto& vkDevice = device.Logical();

  const VkDeviceSize bufferSize = sizeof(mesh.Vertices[0]) * mesh.Vertices.size();
  // Create Staging Buffer.
  const AllocatedBuffer staging =
      device.CreateBuffer(bufferSize,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // Fill Staging Buffer
  void* data{};
  if (vkMapMemory(vkDevice, staging.Memory, 0, bufferSize, 0, &data) != VK_SUCCESS) {
    throw std::runtime_error("Failed to map staging buffer memory");
  }
  std::memcpy(data, mesh.Vertices.data(), bufferSize);
  vkUnmapMemory(vkDevice, staging.Memory);

  // Allocate Vertex Buffer.
  const AllocatedBuffer gpuVertex =
      device.CreateBuffer(bufferSize,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // Staging Buffer to Vertex Buffer.
  device.CopyBuffer(staging.Buffer, gpuVertex.Buffer, bufferSize);
  device.DestroyBuffer(staging);
  return gpuVertex;
}

AllocatedBuffer MeshAllocator::createIndexBuffer_(const Mesh& mesh) const {
  const auto& device = context_.GetDevice();
  const auto& vkDevice = device.Logical();

  const VkDeviceSize bufferSize = sizeof(mesh.Indices[0]) * mesh.Indices.size();
  // Create Staging Buffer.
  const AllocatedBuffer staging =
      device.CreateBuffer(bufferSize,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // Fill Staging Buffer
  void* data{};
  if (vkMapMemory(vkDevice, staging.Memory, 0, bufferSize, 0, &data) != VK_SUCCESS) {
    throw std::runtime_error("Failed to map staging buffer memory");
  }
  std::memcpy(data, mesh.Indices.data(), bufferSize);
  vkUnmapMemory(vkDevice, staging.Memory);

  // Allocate Index Buffer.
  const AllocatedBuffer gpuIndices =
      device.CreateBuffer(bufferSize,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  // Staging Buffer to Index Buffer.
  device.CopyBuffer(staging.Buffer, gpuIndices.Buffer, bufferSize);
  device.DestroyBuffer(staging);
  return gpuIndices;
}
} // namespace engine::graphics::vulkan