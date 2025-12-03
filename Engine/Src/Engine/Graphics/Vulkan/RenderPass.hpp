#pragma once

#include <vulkan/vulkan.h>

namespace engine::graphics::vulkan {

class Context;

class RenderPass {
public:
  explicit RenderPass(Context& context);
  ~RenderPass();

  RenderPass(const RenderPass&) = delete;
  RenderPass& operator=(const RenderPass&) = delete;

  RenderPass(RenderPass&&) noexcept = default;

  VkRenderPass Handle() const noexcept;

private:
  Context& context_;

  VkRenderPass renderPass_{VK_NULL_HANDLE};

  void createRenderPass_();
};

} // namespace engine::graphics::vulkan
