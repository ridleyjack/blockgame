#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace engine::gfx::vulkan {

class Context;

class Sync {
public:
  explicit Sync(Context& context);
  ~Sync();

  Sync(const Sync&) = delete;
  Sync& operator=(const Sync&) = delete;

  void Init();
  void RecreatePerImageSemaphores();

  VkSemaphore& ImageAvailableSemaphore(uint32_t frame);
  VkSemaphore& RenderFinishedSemaphore(uint32_t image);
  VkFence& InFlightFence(uint32_t frame);

private:
  Context& context_;

  std::vector<VkSemaphore> imageAvailableSemaphores_{};
  std::vector<VkSemaphore> renderFinishedSemaphores_{};
  std::vector<VkFence> inFlightFences_{};

  void createPerImageSemaphores_();
  void cleanupPerImageSemaphores_();
};

} // namespace engine::gfx::vulkan
