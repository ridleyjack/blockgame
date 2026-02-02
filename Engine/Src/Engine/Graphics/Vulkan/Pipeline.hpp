#pragma once

#include <expected>
#include <string_view>
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

enum class PipelineError : uint8_t {
  FailedPipelineCreation,
  FailedPipelineLayoutCreation,
  FailedShaderCompile,
};

constexpr std::string_view ToString(const PipelineError e) noexcept {
  using enum PipelineError;

  switch (e) {
  case FailedPipelineCreation:
    return "FailedPipelineCreation";
  case FailedPipelineLayoutCreation:
    return "FailedPipelineLayoutCreation";
  case FailedShaderCompile:
    return "FailedShaderCompile";
  }
  return "UnknownPipelineError";
}

class Pipeline {
public:
  static std::expected<Pipeline, PipelineError>
  Create(Context& context, const RenderPass& renderPass, const PipelineCreateInfo& info, VkDescriptorSetLayout layout);

  Pipeline(Context& context, VkPipelineLayout pipelineLayout, VkPipeline pipeline);
  ~Pipeline();

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;

  Pipeline(Pipeline&& other) noexcept;

  VkPipelineLayout PipelineLayout() const noexcept;
  VkPipeline Handle() const noexcept;

private:
  static std::expected<VkPipelineLayout, PipelineError>
  createPipelineLayout_(const Context& context, VkDescriptorSetLayout descriptorSetLayout) noexcept;

  static std::expected<VkPipeline, PipelineError> createPipeline_(const Context& context,
                                                                  const PipelineCreateInfo& info,
                                                                  const RenderPass& renderPass,
                                                                  VkPipelineLayout pipelineLayout) noexcept;

  static std::expected<VkShaderModule, PipelineError> createShaderModule_(const Context& context,
                                                                          const std::vector<char>& code) noexcept;

  Context& context_;

  VkPipelineLayout pipelineLayout_{VK_NULL_HANDLE};
  VkPipeline pipeline_{VK_NULL_HANDLE};
};

} // namespace engine::graphics::vulkan