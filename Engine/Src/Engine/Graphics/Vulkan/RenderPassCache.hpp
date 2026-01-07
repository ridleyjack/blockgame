#pragma once

#include <expected>
#include <string_view>
#include <vector>
#include <vulkan/vulkan.h>

namespace engine::graphics::vulkan {

class Context;

enum class RenderPassError : uint8_t{
  FailedToCreateRenderPass
};

constexpr std::string_view ToString(const RenderPassError e) noexcept {
  using enum RenderPassError;

  switch (e) {
  case FailedToCreateRenderPass:
    return "FailedToCreateRenderPass";
  }
  return "UnknownRenderError";
}

struct RenderPass {
  VkRenderPass Handle{VK_NULL_HANDLE};
};

class RenderPassCache {
public:
  explicit RenderPassCache(Context& context);
  ~RenderPassCache();

  RenderPassCache(const RenderPassCache&) = delete;
  RenderPassCache& operator=(const RenderPassCache&) = delete;

  RenderPassCache(RenderPassCache&&) noexcept = default;

  std::expected<uint32_t, RenderPassError> CreateRenderPass();
  RenderPass& GetRenderPass(uint32_t renderPassID) noexcept;

private:
  Context& context_;

  std::vector<RenderPass> renderPasses_;

  std::expected<RenderPass, RenderPassError> createRenderPass_() const noexcept;

  VkFormat findDepthFormat_() const;
};

} // namespace engine::graphics::vulkan
