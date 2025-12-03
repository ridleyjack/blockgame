#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace engine::graphics {
struct PipelineCreateInfo;
struct PipelineHandle;
} // namespace engine::graphics

namespace engine::graphics::vulkan {

class Context;
class RenderPass;

class Pipeline {
public:
  Pipeline(Context& context, const PipelineCreateInfo& info, const RenderPass& renderPass);
  ~Pipeline();

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;

  Pipeline(Pipeline&&) noexcept = default;

  VkDescriptorSetLayout DescriptorSetLayout() const noexcept;
  VkPipelineLayout PipelineLayout() const noexcept;
  VkPipeline Handle() const noexcept;

private:
  Context& context_;

  VkDescriptorSetLayout descriptorSetLayout_{VK_NULL_HANDLE};
  VkPipelineLayout pipelineLayout_{VK_NULL_HANDLE};
  VkPipeline pipeline_{VK_NULL_HANDLE};

  void createDescriptorSetLayout_();
  void createPipeline_(const PipelineCreateInfo& info, const RenderPass& renderPass);
  void createPipelineLayout_();

  VkShaderModule createShaderModule_(const std::vector<char>& code) const;
};

} // namespace engine::graphics::vulkan