#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>

namespace engine::graphics {

struct Vertex;
struct Mesh;

namespace vulkan {
class Context;
}

class RenderObject {
public:
  explicit RenderObject(vulkan::Context& context);
  ~RenderObject();

  RenderObject(const RenderObject&) = delete;
  RenderObject& operator=(const RenderObject&) = delete;

  void Record(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
  void UploadMesh(Mesh& mesh);

private:
  vulkan::Context& context_;

  uint32_t verticesSize_{};
  VkBuffer vertexBuffer_{VK_NULL_HANDLE};
  VkDeviceMemory vertexBufferMemory_{VK_NULL_HANDLE};
  // TODO: It's recommended to put index and vertex buffer in same memory for better caching.
  uint32_t indicesSize_{};
  VkBuffer indexBuffer_{VK_NULL_HANDLE};
  VkDeviceMemory indexBufferMemory_{VK_NULL_HANDLE};

  void createVertexBuffer_(const std::vector<Vertex>& vertices);
  void createIndexBuffer_(std::vector<uint32_t> indices);
  void createBuffer_(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                     VkDeviceMemory& bufferMemory);
  void copyBuffer_(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
};

} // namespace engine::graphics