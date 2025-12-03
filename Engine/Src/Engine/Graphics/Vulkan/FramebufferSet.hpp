#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace engine::graphics::vulkan {

class Context;
class RenderPass;
class SwapChain;

class FramebufferSet {
public:
  FramebufferSet(Context& context, const SwapChain& swapChain, const RenderPass& renderPass);
  ~FramebufferSet();

  VkFramebuffer Framebuffer(int frameID) const noexcept;

private:
  Context& context_;
  std::vector<VkFramebuffer> framebuffers_{};
};

} // namespace engine::graphics::vulkan