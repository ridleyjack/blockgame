#include "TextureAllocator.hpp"

#include "Context.hpp"
#include "Device.hpp"
#include "ImageUtil.hpp"
#include "StagingBuffer.hpp"
#include "Uploader.hpp"

#include "Engine/Graphics/TextureArrayInfo.hpp"

#include <cassert>

namespace engine::graphics::vulkan {

TextureAllocator::TextureAllocator(Context& context, Uploader& uploader, StagingBuffer& staging)
    : context_(context), uploader_(uploader), stagingBuffer_(staging) {}

TextureAllocator::~TextureAllocator() {

  for (auto& texture : textures_) {
    cleanupGPUTexture_(texture);
  }

  if (arrayState_.has_value()) {
    cleanupGPUTexture_(arrayState_->Texture);
  }
}

std::expected<std::uint32_t, TextureError> TextureAllocator::Create(const std::span<const std::byte>& imageData,
                                                                    const std::uint32_t width,
                                                                    const std::uint32_t height) {
  constexpr std::uint32_t numLayers = 1;
  auto [cmd, batchID] = uploader_.GetCurrent();
  // TODO: If texture creation encounters an error we still record copy commands on now freed buffers. We need to cancel
  // the whole command or not free the buffers until execution is complete.

  TextureGPU texture{};
  if (auto result = createImage_(cmd, width, height, numLayers); !result) {
    return std::unexpected(result.error());
  } else {
    texture = *result;
  }

  uploadLayer_(texture, cmd, batchID, width, height, 0, imageData);

  std::uint32_t textureID{};
  if (auto result = finishTexture_(cmd, texture, numLayers); !result) {
    return std::unexpected(result.error());
  } else {
    textureID = *result;
  }

  Uploader::UploadRequest request{.OnComplete = [this, textureID]() noexcept {
    TextureGPU& texture = textures_[textureID];
    if (texture.State != TextureState::Failed)
      texture.State = TextureState::Ready;
  }};
  uploader_.Queue(std::move(request));

  return textureID;
}

const TextureGPU& TextureAllocator::Get(const std::uint32_t textureID) const noexcept {
  assert(textureID < textures_.size());
  return textures_[textureID];
}

std::expected<void, TextureError> TextureAllocator::BeginArray(const TextureArrayInfo& info) noexcept {
  assert(!arrayState_.has_value());
  arrayState_.emplace(ArrayBuildState{.LayerSizeBytes = info.LayerSizeBytes,
                                      .Width = info.Width,
                                      .Height = info.Height,
                                      .NumLayers = info.NumLayers});

  auto [cmd, batchID] = uploader_.GetCurrent();
  TextureGPU& texture = arrayState_->Texture;

  if (auto result = createImage_(cmd, info.Width, info.Height, info.NumLayers); !result) {
    return std::unexpected(result.error());
  } else {
    texture = *result;
  }

  uploader_.Queue({});
  return {};
}

void TextureAllocator::UploadLayer(const std::span<const std::byte>& imageData) {
  assert(arrayState_.has_value());
  assert(arrayState_->NextLayer < arrayState_->NumLayers);
  assert(imageData.size() == arrayState_->LayerSizeBytes);

  auto [cmd, batchID] = uploader_.GetCurrent();
  uploadLayer_(arrayState_->Texture,
               cmd,
               batchID,
               arrayState_->Width,
               arrayState_->Height,
               arrayState_->NextLayer,
               imageData);

  Uploader::UploadRequest request{.OnComplete = []() noexcept {}};
  uploader_.Queue(std::move(request));

  arrayState_->NextLayer++;
}

std::expected<std::uint32_t, TextureError> TextureAllocator::FinishArray() {
  assert(arrayState_.has_value());
  assert(arrayState_->NextLayer == arrayState_->NumLayers);

  std::uint32_t textureID{};
  auto [cmd, batchID] = uploader_.GetCurrent();
  if (auto result = finishTexture_(cmd, arrayState_->Texture, arrayState_->NumLayers); !result) {
    return std::unexpected(result.error());
  } else {
    textureID = *result;
  }

  Uploader::UploadRequest request{.OnComplete = [this, textureID]() noexcept {
    TextureGPU& texture = textures_[textureID];
    if (texture.State != TextureState::Failed)
      texture.State = TextureState::Ready;
  }};
  uploader_.Queue(std::move(request));

  arrayState_.reset();
  return textureID;
}

std::expected<TextureGPU, TextureError> TextureAllocator::createImage_(VkCommandBuffer cmd,
                                                                       const std::uint32_t width,
                                                                       const std::uint32_t height,
                                                                       const std::uint32_t numLayers) const noexcept {

  const auto& device = context_.GetDevice();

  TextureGPU texture{};
  // Create Image.
  if (auto result = imageutil::CreateImage(device,
                                           {.Width = width,
                                            .Height = height,
                                            .ArrayLayers = numLayers,
                                            .Format = VK_FORMAT_R8G8B8A8_SRGB,
                                            .Tiling = VK_IMAGE_TILING_OPTIMAL,
                                            .Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                            .Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT});
      !result) {
    return std::unexpected(TextureError{.Code = TextureError::ErrorCode::ImageUtilFailure, .Cause = result.error()});
  } else {
    texture.Image = result->Handle;
    texture.ImageMemory = result->Memory;
  }

  // Transition new image for writing.
  if (auto result = imageutil::TransitionImageLayout(cmd,
                                                     texture.Image,
                                                     VK_FORMAT_R8G8B8A8_SRGB,
                                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                     numLayers);
      !result) {
    cleanupGPUTexture_(texture);
    return std::unexpected(TextureError{.Code = TextureError::ErrorCode::ImageUtilFailure, .Cause = result.error()});
  }

  return texture;
}

void TextureAllocator::uploadLayer_(const TextureGPU& texture,
                                    VkCommandBuffer cmd,
                                    const std::uint64_t batchID,
                                    const std::uint32_t width,
                                    const std::uint32_t height,
                                    const std::uint32_t layerIndex,
                                    const std::span<const std::byte>& bytes) const noexcept {
  VkDeviceSize offset = stagingBuffer_.Write(bytes, 1, batchID);
  copyBufferToImage_(cmd, offset, texture.Image, width, height, layerIndex);
}

std::expected<std::uint32_t, TextureError>
TextureAllocator::finishTexture_(VkCommandBuffer cmd, TextureGPU& texture, const std::uint32_t numLayers) noexcept {
  if (auto result = imageutil::TransitionImageLayout(cmd,
                                                     texture.Image,
                                                     VK_FORMAT_R8G8B8A8_SRGB,
                                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                     numLayers);
      !result) {
    cleanupGPUTexture_(texture);
    return std::unexpected(TextureError{.Code = TextureError::ErrorCode::ImageUtilFailure, .Cause = result.error()});
  }

  // Create the Imageview.
  if (auto result = createTextureImageView_(texture.Image, VK_FORMAT_R8G8B8A8_SRGB, numLayers); !result) {
    cleanupGPUTexture_(texture);
    return std::unexpected(result.error());
  } else {
    texture.ImageView = *result;
  }

  // Create the Sampler.
  if (auto result = createSampler_(); !result) {
    cleanupGPUTexture_(texture);
    return std::unexpected(result.error());
  } else {
    texture.Sampler = *result;
  }

  textures_.push_back(texture);
  return textures_.size() - 1;
}

void TextureAllocator::copyBufferToImage_(VkCommandBuffer cmd,
                                          VkDeviceSize stagingOffset,
                                          VkImage image,
                                          const uint32_t width,
                                          const uint32_t height,
                                          const uint32_t layerIndex) const noexcept {
  VkBufferImageCopy region{};
  region.bufferOffset = stagingOffset;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = layerIndex;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(cmd, stagingBuffer_.Handle(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

std::expected<VkImageView, TextureError>
TextureAllocator::createTextureImageView_(VkImage image, VkFormat format, uint32_t arrayLayers) const noexcept {
  auto result = imageutil::CreateImageView(
      context_.GetDevice(),
      {.ArrayLayers = arrayLayers, .Format = format, .AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT, .Image = image});
  if (!result)
    return std::unexpected(TextureError{.Code = TextureError::ErrorCode::ImageUtilFailure, .Cause = result.error()});
  return *result;
}

std::expected<VkSampler, TextureError> TextureAllocator::createSampler_() const noexcept {
  const auto& device = context_.GetDevice();

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device.Physical(), &properties);
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  VkSampler sampler;
  if (vkCreateSampler(device.Logical(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
    return std::unexpected(TextureError{.Code = TextureError::ErrorCode::FailedSamplerCreation});
  }

  return sampler;
}

void TextureAllocator::cleanupGPUTexture_(TextureGPU& texture) const noexcept {
  auto vkDevice = context_.GetDevice().Logical();

  vkDestroySampler(vkDevice, texture.Sampler, nullptr);
  vkDestroyImageView(vkDevice, texture.ImageView, nullptr);

  vkDestroyImage(vkDevice, texture.Image, nullptr);
  vkFreeMemory(vkDevice, texture.ImageMemory, nullptr);

  texture = {};
}

} // namespace engine::graphics::vulkan