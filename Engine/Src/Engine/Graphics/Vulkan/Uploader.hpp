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
  Uploader(Context& context, StagingBuffer& stagingBuffer);
  ~Uploader();

  struct UploadRequest {
    std::move_only_function<void(VkCommandBuffer, std::uint64_t batchID)> Record;
    std::move_only_function<void() noexcept> OnComplete;
  };

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
};

} // namespace engine::graphics::vulkan