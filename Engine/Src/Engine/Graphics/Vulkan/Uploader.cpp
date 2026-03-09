#include "Uploader.hpp"

#include "Context.hpp"
#include "Device.hpp"
#include "Command.hpp"
#include "StagingBuffer.hpp"

namespace engine::graphics::vulkan {

Uploader::Uploader(Context& context, StagingBuffer& stagingBuffer) : context_(context), stagingBuffer_(stagingBuffer) {}

Uploader::~Uploader() {
  const auto& cmd = context_.GetCommand();
  auto vkDevice = context_.GetDevice().Logical();

  auto freeBatch = [&](const UploadBatch& batch) {
    vkDestroyFence(vkDevice, batch.Fence, nullptr);
    cmd.FreeTransient(batch.Command);
  };

  if (pending_.has_value()) {
    freeBatch(*pending_);
  }
  for (auto& batch : submitted_) {
    freeBatch(batch);
  }
}

void Uploader::Queue(UploadRequest request) {
  const auto& cmd = context_.GetCommand();
  auto vkDevice = context_.GetDevice().Logical();

  if (!pending_.has_value()) {
    VkFenceCreateInfo fenceInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

    VkFence fence{VK_NULL_HANDLE};
    if (vkCreateFence(vkDevice, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
      throw std::runtime_error("failed to create uploader fence!");
    }

    pending_.emplace(
        UploadBatch{.Fence = fence, .Command = cmd.BeginTransient(), .BatchID = stagingBuffer_.BeginBatch()});
  }
  request.Record(pending_->Command, pending_->BatchID);
  pending_->OnComplete.emplace_back(std::move(request.OnComplete));
}

void Uploader::Process() {
  const auto& cmd = context_.GetCommand();
  const auto& device = context_.GetDevice();
  auto vkDevice = device.Logical();

  // Process completed uploads
  for (auto it = submitted_.begin(); it != submitted_.end();) {
    if (vkGetFenceStatus(vkDevice, it->Fence) == VK_NOT_READY) {
      ++it;
      continue;
    }

    for (auto& callback : it->OnComplete) {
      callback();
    }

    vkDestroyFence(vkDevice, it->Fence, nullptr);
    cmd.FreeTransient(it->Command);
    stagingBuffer_.Free(it->BatchID);
    it = submitted_.erase(it);
  }

  // Submit pending uploads.
  if (!pending_.has_value())
    return;

  const VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                .commandBufferCount = 1,
                                .pCommandBuffers = &pending_->Command};
  vkEndCommandBuffer(pending_->Command);
  vkQueueSubmit(device.GraphicsQueue(), 1, &submitInfo, pending_->Fence);

  submitted_.push_back(std::move(*pending_));
  pending_.reset();
}

} // namespace engine::graphics::vulkan