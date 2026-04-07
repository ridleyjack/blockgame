#include "TextureAllocator.hpp"

#include "Context.hpp"
#include "Device.hpp"
#include "Image.hpp"
#include "StagingBuffer.hpp"
#include "Uploader.hpp"

#include "../Resources/TextureArrayInfo.hpp"

#include <cassert>

namespace engine::graphics::vulkan {

TextureAllocator::TextureAllocator(Context& context, Uploader& uploader, StagingBuffer& staging)
    : context_(context), uploader_(uploader), stagingBuffer_(staging) {

  // Ensure GPU supports linear blitting which is required for mipmap generation.
  constexpr VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

  auto vkDevicePhysical = context_.GetDevice().Physical();
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(vkDevicePhysical, imageFormat, &formatProperties);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    throw std::runtime_error("texture image format does not support linear blitting!");
  }
}

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

  uploadLayer_(texture, cmd, batchID, 0, imageData);

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

std::expected<void, TextureError> TextureAllocator::BeginArray(const resources::TextureArrayInfo& info) noexcept {
  assert(!arrayState_.has_value());
  arrayState_.emplace(ArrayBuildState{.LayerSizeBytes = info.LayerSizeBytes, .NumLayers = info.NumLayers});

  auto [cmd, batchID] = uploader_.GetCurrent();
  TextureGPU& texture = arrayState_->Texture;

  if (auto result = createImage_(cmd, info.Width, info.Height, info.NumLayers); !result) {
    arrayState_.reset();
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
  uploadLayer_(arrayState_->Texture, cmd, batchID, arrayState_->NextLayer, imageData);

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

  const std::uint32_t mipLevels = image::CalculateMipLevels(width, height);
  TextureGPU texture{.Width = width, .Height = height, .MipLevels = mipLevels};
  const image::ImageSpec spec{
      .Width = width,
      .Height = height,
      .MipLevels = mipLevels,
      .ArrayLayers = numLayers,

      .Format = VK_FORMAT_R8G8B8A8_SRGB,
      .Tiling = VK_IMAGE_TILING_OPTIMAL,
      .Usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
  };

  // Create Image.
  if (auto result = image::CreateImage(device, spec); !result) {
    return std::unexpected(TextureError{.Code = TextureError::ErrorCode::ImageUtilFailure, .Cause = result.error()});
  } else {
    texture.Image = result->Handle;
    texture.ImageMemory = result->Memory;
  }

  // Transition new image for writing.
  if (auto result = image::TransitionImageLayout(cmd,
                                                 texture.Image,
                                                 spec,
                                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
      !result) {
    cleanupGPUTexture_(texture);
    return std::unexpected(TextureError{.Code = TextureError::ErrorCode::ImageUtilFailure, .Cause = result.error()});
  }

  return texture;
}

void TextureAllocator::uploadLayer_(const TextureGPU& texture,
                                    VkCommandBuffer cmd,
                                    const std::uint64_t batchID,
                                    const std::uint32_t layerIndex,
                                    const std::span<const std::byte>& bytes) const noexcept {
  const VkDeviceSize offset = stagingBuffer_.Write(bytes, 1, batchID);
  copyBufferToImage_(cmd, offset, texture.Image, texture.Width, texture.Height, layerIndex);
  generateMipmaps_(cmd, texture.Image, texture.Width, texture.Height, layerIndex, texture.MipLevels);
}

std::expected<std::uint32_t, TextureError>
TextureAllocator::finishTexture_(VkCommandBuffer cmd,
                                 TextureGPU& texture,
                                 std::uint32_t numLayers) noexcept { // Create the Imageview.
  const image::ImageViewSpec spec{
      .ArrayLayers = numLayers,
      .MipLevels = texture.MipLevels,
      .Format = VK_FORMAT_R8G8B8A8_SRGB,
      .AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
      .ViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,

  };
  if (auto result = createTextureImageView_(texture.Image, spec); !result) {
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
                                          const std::uint32_t width,
                                          const std::uint32_t height,
                                          const std::uint32_t layerIndex) const noexcept {
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
TextureAllocator::createTextureImageView_(VkImage image, const image::ImageViewSpec& spec) const noexcept {
  auto result = image::CreateImageView(context_.GetDevice(), image, spec);
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
  samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

  VkSampler sampler;
  if (vkCreateSampler(device.Logical(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
    return std::unexpected(TextureError{.Code = TextureError::ErrorCode::FailedSamplerCreation});
  }

  return sampler;
}

auto TextureAllocator::generateMipmaps_(VkCommandBuffer cmd,
                                        VkImage image,
                                        const std::uint32_t width,
                                        const std::uint32_t height,
                                        const std::uint32_t layerIndex,
                                        const std::uint32_t mipLevels) const noexcept -> void {

  VkImageMemoryBarrier barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = {
                           .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .levelCount = 1,
                           .baseArrayLayer = layerIndex,
                           .layerCount = 1,
                           }
  };

  std::int32_t mipWidth = static_cast<std::int32_t>(width);
  std::int32_t mipHeight = static_cast<std::int32_t>(height);
  for (std::uint32_t i = 1; i < mipLevels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    VkImageBlit blit{
        .srcSubresource =
            {
                             .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .mipLevel = i - 1,
                             .baseArrayLayer = layerIndex,
                             .layerCount = 1,
                             },
        .srcOffsets = {{0, 0, 0}, {mipWidth, mipHeight, 1}},
        .dstSubresource =
            {
                             .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .mipLevel = i,
                             .baseArrayLayer = layerIndex,
                             .layerCount = 1,
                             },
        .dstOffsets = {{0, 0, 0}, {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1}},
    };
    vkCmdBlitImage(cmd,
                   image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1,
                   &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    if (mipWidth > 1)
      mipWidth /= 2;
    if (mipHeight > 1)
      mipHeight /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(cmd,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);
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