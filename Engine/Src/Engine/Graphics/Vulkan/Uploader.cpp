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

  if (pending_) {
    freeBatch(*pending_);
  }
  for (auto& batch : submitted_) {
    freeBatch(batch);
  }
}

Uploader::UploadContext Uploader::GetCurrent() {
  if (!pending_)
    createBatch_();

  return UploadContext{.Command = pending_->Command, .BatchID = pending_->BatchID};
}

void Uploader::Queue(UploadRequest request) {
  if (!pending_)
    createBatch_();

  if (request.OnComplete)
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
      if (callback)
        callback();
    }

    vkDestroyFence(vkDevice, it->Fence, nullptr);
    cmd.FreeTransient(it->Command);
    stagingBuffer_.Free(it->BatchID);
    it = submitted_.erase(it);
  }

  // Submit pending uploads.
  if (!pending_)
    return;

  const VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                .commandBufferCount = 1,
                                .pCommandBuffers = &pending_->Command};
  if (vkEndCommandBuffer(pending_->Command) != VK_SUCCESS)
    throw std::runtime_error("failed to end uploader command!");
  if (vkQueueSubmit(device.GraphicsQueue(), 1, &submitInfo, pending_->Fence) != VK_SUCCESS)
    throw std::runtime_error("failed to submit uploader command!");

  submitted_.push_back(std::move(*pending_));
  pending_.reset();
}

void Uploader::createBatch_() {
  const auto& cmd = context_.GetCommand();
  auto vkDevice = context_.GetDevice().Logical();

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

} // namespace engine::graphics::vulkan