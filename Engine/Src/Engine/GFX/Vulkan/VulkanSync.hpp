#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace Engine {

class Context;

class Sync {
public:
  explicit Sync(Context& context);
  ~Sync();

  Sync(const Sync&) = delete;
  Sync& operator=(const Sync&) = delete;

  void Init();

  void CreatePerImageSemaphores();
  void DestroyPerImageSemaphores();

  VkSemaphore& ImageAvailableSemaphore(uint32_t frame);
  VkSemaphore& RenderFinishedSemaphore(uint32_t frame);
  VkFence& InFlightFence(uint32_t frame);

private:
  Context& context_;

  std::vector<VkSemaphore> imageAvailableSemaphores_{};
  std::vector<VkSemaphore> renderFinishedSemaphores_{};
  std::vector<VkFence> inFlightFences_{};
};

} // namespace oc::graphics::vulkan
