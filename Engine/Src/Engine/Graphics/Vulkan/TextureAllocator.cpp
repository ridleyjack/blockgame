#include "TextureAllocator.hpp"

#include "Command.hpp"
#include "Context.hpp"
#include "Device.hpp"
#include "ImageUtil.hpp"

#include "Engine/Graphics/TextureArrayInfo.hpp"

#include <vulkan/vulkan.h>

#include <cassert>
#include <cstring>

namespace engine::graphics::vulkan {

TextureAllocator::TextureAllocator(Context& context) : context_(context) {}
TextureAllocator::~TextureAllocator() {
  const auto& device = context_.GetDevice();

  for (auto& texture : textures_) {
    const auto& vkDevice = context_.GetDevice().Logical();
    cleanupGPUTexture_(texture);
  }

  if (arrayState_.has_value()) {
    cleanupGPUTexture_(arrayState_->Texture);
    vkUnmapMemory(device.Logical(), arrayState_->Staging.Buffer.Memory);
    device.DestroyBuffer(arrayState_->Staging.Buffer);
  }
}

std::expected<uint32_t, TextureError>
TextureAllocator::Create(const std::span<const std::byte>& imageData, const uint32_t width, const uint32_t height) {
  constexpr std::uint32_t numLayers = 1;

  TextureGPU texture{};
  if (auto result = createImage_(width, height, numLayers); !result) {
    return std::unexpected(result.error());
  } else {
    texture = *result;
  }

  TextureStaging staging = createStaging_(imageData.size_bytes());
  uploadLayer_(texture, staging, width, height, imageData.size_bytes(), 0, imageData);

  std::uint32_t textureID{};
  if (auto result = finishTexture_(texture, staging, numLayers); !result) {
    return std::unexpected(result.error());
  } else {
    textureID = *result;
  }
  return textureID;
}

const TextureGPU& TextureAllocator::Get(const uint32_t textureID) const noexcept {
  assert(textureID < textures_.size());
  return textures_[textureID];
}

std::expected<void, TextureError> TextureAllocator::BeginArray(const TextureArrayInfo& info) noexcept {
  assert(!arrayState_.has_value());
  TextureGPU texture{};
  if (auto result = createImage_(info.Width, info.Height, info.NumLayers); !result) {
    return std::unexpected(result.error());
  } else {
    texture = *result;
  }

  const TextureStaging staging = createStaging_(info.LayerSizeBytes);

  arrayState_.emplace(ArrayBuildState{.Texture = texture,
                                      .Staging = staging,
                                      .LayerSizeBytes = info.LayerSizeBytes,
                                      .Width = info.Width,
                                      .Height = info.Height,
                                      .NumLayers = info.NumLayers});
  return {};
}

void TextureAllocator::UploadLayer(const std::span<const std::byte>& imageData) {
  assert(arrayState_.has_value());
  assert(arrayState_->NextLayer < arrayState_->NumLayers);
  assert(imageData.size() == arrayState_->LayerSizeBytes);

  uploadLayer_(arrayState_->Texture,
               arrayState_->Staging,
               arrayState_->Width,
               arrayState_->Height,
               arrayState_->LayerSizeBytes,
               arrayState_->NextLayer,
               imageData);
  arrayState_->NextLayer++;
}

std::expected<std::uint32_t, TextureError> TextureAllocator::FinishArray() {
  assert(arrayState_.has_value());
  assert(arrayState_->NextLayer == arrayState_->NumLayers);

  std::uint32_t textureID{};
  if (auto result = finishTexture_(arrayState_->Texture, arrayState_->Staging, arrayState_->NumLayers); !result) {
    arrayState_.reset();
    return std::unexpected(result.error());
  } else {
    textureID = *result;
  }
  arrayState_.reset();
  return textureID;
}

std::expected<TextureGPU, TextureError> TextureAllocator::createImage_(const std::uint32_t width,
                                                                       const std::uint32_t height,
                                                                       const std::uint32_t numLayers) const noexcept {

  const auto& device = context_.GetDevice();
  const auto& cmd = context_.GetCommand();

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

TextureStaging TextureAllocator::createStaging_(const VkDeviceSize layerSizeBytes) const noexcept {
  const auto& device = context_.GetDevice();

  const AllocatedBuffer staging =
      device.CreateBuffer(layerSizeBytes,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void* data;
  vkMapMemory(device.Logical(), staging.Memory, 0, layerSizeBytes, 0, &data);

  return {.Buffer = staging, .Data = data};
}

void TextureAllocator::uploadLayer_(const TextureGPU& texture,
                                    const TextureStaging& staging,
                                    const std::uint32_t width,
                                    const std::uint32_t height,
                                    const VkDeviceSize layerSizeBytes,
                                    const std::uint32_t layerIndex,
                                    const std::span<const std::byte>& bytes) const noexcept {
  memcpy(staging.Data, bytes.data(), layerSizeBytes);
  copyBufferToImage_(staging.Buffer.Buffer, texture.Image, width, height, layerIndex);
}

std::expected<std::uint32_t, TextureError> TextureAllocator::finishTexture_(TextureGPU& texture,
                                                                            const TextureStaging& staging,
                                                                            const std::uint32_t numLayers) noexcept {
  const auto& device = context_.GetDevice();
  auto vkDevice = device.Logical();
  const auto& cmd = context_.GetCommand();

  // Cleanup Buffer, method guarantees this is done.
  vkUnmapMemory(vkDevice, staging.Buffer.Memory);
  device.DestroyBuffer(staging.Buffer);

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

  // Store the texture.
  textures_.push_back(texture);
  return textures_.size() - 1;
}

void TextureAllocator::copyBufferToImage_(VkBuffer buffer,
                                          VkImage image,
                                          const uint32_t width,
                                          const uint32_t height,
                                          const uint32_t layerIndex) const noexcept {
  const auto& cmd = context_.GetCommand();
  // TODO: Batch these commands.
  VkCommandBuffer commandBuffer = cmd.BeginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = layerIndex;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  cmd.EndSingleTimeCommands(commandBuffer);
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