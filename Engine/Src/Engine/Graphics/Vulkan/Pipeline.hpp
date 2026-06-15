#pragma once

#include "Engine/Graphics/PipelineCreateInfo.hpp"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vector>
#include <expected>
#include <string_view>
#include <span>

namespace gfx = engine::graphics;

namespace engine::graphics {
struct PipelineCreateInfo;
struct PipelineHandle;
} // namespace engine::graphics

namespace engine::graphics::vulkan {

class Context;

enum class PipelineError : uint8_t {
  PipelineCreation,
  PipelineLayoutCreation,
  ShaderCompile,
  ShaderFileRead,
};

constexpr std::string_view ToString(const PipelineError e) noexcept {
  using enum PipelineError;

  switch (e) {
  case PipelineCreation:
    return "FailedPipelineCreation";
  case PipelineLayoutCreation:
    return "FailedPipelineLayoutCreation";
  case ShaderCompile:
    return "FailedShaderCompile";
  case ShaderFileRead:
    return "FailedShaderFileRead";
  }
  return "UnknownPipelineError";
}

class Pipeline {
public:
  static std::expected<Pipeline, PipelineError>
  Create(Context& context, const PipelineCreateInfo& info, std::span<const VkDescriptorSetLayout> layouts);

  Pipeline(Context& context, const PipelineCreateInfo& info, VkPipelineLayout pipelineLayout, VkPipeline pipeline);
  ~Pipeline();

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;

  Pipeline(Pipeline&& other) noexcept;

  VkPipelineLayout PipelineLayout() const noexcept;
  VkPipeline Handle() const noexcept;

  PipelineKind Kind() const noexcept;

private:
  static std::expected<VkPipelineLayout, PipelineError>
  createPipelineLayout_(const Context& context, std::span<const VkDescriptorSetLayout> layouts) noexcept;

  static std::expected<VkPipeline, PipelineError>
  createPipeline_(const Context& context, const PipelineCreateInfo& info, VkPipelineLayout pipelineLayout) noexcept;

  static std::expected<VkShaderModule, PipelineError>
  createShaderModule_(const Context& context, const std::vector<std::uint32_t>& code) noexcept;

  Context& context_;

  VkPipelineLayout pipelineLayout_{VK_NULL_HANDLE};
  VkPipeline pipeline_{VK_NULL_HANDLE};

  gfx::PipelineKind kind_;
};

} // namespace engine::graphics::vulkan
