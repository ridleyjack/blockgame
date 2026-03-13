#pragma once

#include <atomic>
#include <vulkan/vulkan_core.h>

#include <vector>
#include <optional>
#include <functional>

namespace engine::graphics::vulkan {

class Context;
class StagingBuffer;

class Uploader {
public:
  struct UploadRequest {
    std::move_only_function<void() noexcept> OnComplete;
  };
  struct UploadContext {
    VkCommandBuffer Command{VK_NULL_HANDLE};
    std::uint64_t BatchID{};
  };

  Uploader(Context& context, StagingBuffer& stagingBuffer);
  ~Uploader();

  UploadContext GetCurrent();
  void Queue(UploadRequest request);
  void Process();

private:
  struct UploadBatch {
    VkFence Fence{VK_NULL_HANDLE};
    VkCommandBuffer Command{VK_NULL_HANDLE};

    std::uint64_t BatchID{};
    std::vector<std::move_only_function<void() noexcept>> OnComplete;
  };

  Context& context_;
  StagingBuffer& stagingBuffer_;

  std::optional<UploadBatch> pending_{};
  std::vector<UploadBatch> submitted_{};

  void createBatch_();
};

} // namespace engine::graphics::vulkan