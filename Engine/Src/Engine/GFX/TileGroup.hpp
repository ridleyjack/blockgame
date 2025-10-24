#pragma once

#define GLM_FORCE_RADIANS

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <vector>

namespace engine::gfx {

namespace vulkan {
class Context;
}

struct Vertex {
  glm::vec2 Pos;
  glm::vec3 Color;
  glm::vec2 TexCoord;

  static VkVertexInputBindingDescription GetBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, Pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, Color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, TexCoord);

    return attributeDescriptions;
  }
};

const std::vector<Vertex> Vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f},   {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f},  {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> Indices = {0, 1, 2, 2, 3, 0};

struct UniformBufferObject {
  alignas(16) glm::mat4 Model;
  alignas(16) glm::mat4 View;
  alignas(16) glm::mat4 Proj;
};

class TileGroup {
public:
  explicit TileGroup(vulkan::Context& context);
  ~TileGroup();

  TileGroup(const TileGroup&) = delete;
  TileGroup& operator=(const TileGroup&) = delete;

  void Init();

  void UpdateUniformBuffer(uint32_t currentImage);

  void Record(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);

private:
  vulkan::Context& context_;

  VkBuffer vertexBuffer_{VK_NULL_HANDLE};
  VkDeviceMemory vertexBufferMemory_{VK_NULL_HANDLE};
  // TODO: It's recommended to put index and vertex buffer in same memory for better caching.
  VkBuffer indexBuffer_{VK_NULL_HANDLE};
  VkDeviceMemory indexBufferMemory_{VK_NULL_HANDLE};

  std::vector<VkBuffer> uniformBuffers_{};
  std::vector<VkDeviceMemory> uniformBuffersMemory_{};
  std::vector<void*> uniformBuffersMapped_{};

  VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};
  std::vector<VkDescriptorSet> descriptorSets_{};

  VkImage textureImage_{VK_NULL_HANDLE};
  VkDeviceMemory textureImageMemory_{VK_NULL_HANDLE};
  VkImageView textureImageView_{VK_NULL_HANDLE};
  VkSampler textureSampler_{VK_NULL_HANDLE};

  void createTextureImage_();
  void createTextureImageView_();
  void createTextureSampler_();
  void createVertexBuffer_();
  void createIndexBuffer_();
  void createUniformBuffers_();
  void createDescriptorPool_();
  void createDescriptorSets_();

  void createBuffer_(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                     VkDeviceMemory& bufferMemory);

  void copyBuffer_(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

  void createImage_(int32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

  void transitionImageLayout_(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
  void copyBufferToImage_(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
};

} // namespace Engine::GFX
