#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace engine::gfx::vulkan {

class Context;
struct CreatePipelineRequest;

class Pipeline {
public:
  explicit Pipeline(Context& context);
  ~Pipeline();

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;

  void Create(const CreatePipelineRequest& request);

  VkRenderPass RenderPass() const;
  VkDescriptorSetLayout DescriptorSetLayout() const;
  VkPipelineLayout PipelineLayout() const;
  VkPipeline GraphicsPipeline() const;

private:
  Context& context_;

  VkRenderPass renderPass_{VK_NULL_HANDLE};
  VkDescriptorSetLayout descriptorSetLayout_{VK_NULL_HANDLE};
  VkPipelineLayout pipelineLayout_{VK_NULL_HANDLE};
  VkPipeline graphicsPipeline_{VK_NULL_HANDLE};

  void createRenderPass_();
  void createDescriptorSetLayout_();
  void createGraphicsPipeline_(const CreatePipelineRequest& request);

  VkShaderModule createShaderModule_(const std::vector<char>& code) const;
};

} // namespace Engine::GFX::Vulkan
// oc