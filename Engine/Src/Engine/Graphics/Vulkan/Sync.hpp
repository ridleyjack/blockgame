#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace engine::graphics::vulkan {

class Context;

class Sync {
public:
  explicit Sync(Context& context);
  ~Sync();

  Sync(const Sync&) = delete;
  Sync& operator=(const Sync&) = delete;

  void RecreatePerImageSemaphores();

  VkSemaphore& ImageAvailableSemaphore(uint32_t frame) noexcept;
  VkSemaphore& RenderFinishedSemaphore(uint32_t image) noexcept;
  VkFence& InFlightFence(uint32_t frame) noexcept;

private:
  Context& context_;

  std::vector<VkSemaphore> imageAvailableSemaphores_{};
  std::vector<VkSemaphore> renderFinishedSemaphores_{};
  std::vector<VkFence> inFlightFences_{};

  void createPerImageSemaphores_();
  void cleanupPerImageSemaphores_();
};

} // namespace engine::graphics::vulkan
