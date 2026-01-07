#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

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
  Pipeline(Context& context,
           const PipelineCreateInfo& info,
           const RenderPass& renderPass,
           VkDescriptorSetLayout descriptorLayout);
  ~Pipeline();

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;

  Pipeline(Pipeline&&) noexcept = default;
  VkPipelineLayout PipelineLayout() const noexcept;
  VkPipeline Handle() const noexcept;

private:
  Context& context_;

  VkPipelineLayout pipelineLayout_{VK_NULL_HANDLE};
  VkPipeline pipeline_{VK_NULL_HANDLE};

  void createPipelineLayout_(VkDescriptorSetLayout descriptorLayout);
  void createPipeline_(const PipelineCreateInfo& info, const RenderPass& renderPass);

  VkShaderModule createShaderModule_(const std::vector<char>& code) const;
};

} // namespace engine::graphics::vulkan