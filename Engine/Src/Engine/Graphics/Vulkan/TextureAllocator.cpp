#include "TextureAllocator.h"

#include "Command.hpp"
#include "Context.hpp"
#include "Device.hpp"
#include "ImageUtil.hpp"

#include <cassert>
#include <vulkan/vulkan.h>

#include <cstring>

namespace engine::graphics::vulkan {

TextureAllocator::TextureAllocator(Context& context) : context_(context) {}
TextureAllocator::~TextureAllocator() {
  for (auto& texture : textures_) {
    const auto& vkDevice = context_.GetDevice().Logical();
    vkDestroyImage(vkDevice, texture.Image, nullptr);
    vkFreeMemory(vkDevice, texture.ImageMemory, nullptr);

    vkDestroyImageView(vkDevice, texture.ImageView, nullptr);
    vkDestroySampler(vkDevice, texture.Sampler, nullptr);
  }
}

std::expected<uint32_t, TextureError>
TextureAllocator::Create(const std::span<const std::byte> pixels, const uint32_t width, const uint32_t height) {
  const auto& device = context_.GetDevice();
  VkDevice vkDevice = device.Logical();
  const auto& cmd = context_.GetCommand();

  const VkDeviceSize imageSize = pixels.size_bytes();
  TextureGPU texture{};

  // Image data to Staging Buffer.
  const AllocatedBuffer staging =
      device.CreateBuffer(imageSize,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void* data;
  vkMapMemory(vkDevice, staging.Memory, 0, imageSize, 0, &data);
  memcpy(data, pixels.data(), imageSize);
  vkUnmapMemory(vkDevice, staging.Memory);

  if (auto result = imageutil::CreateImage(device,
                                           width,
                                           height,
                                           VK_FORMAT_R8G8B8A8_SRGB,
                                           VK_IMAGE_TILING_OPTIMAL,
                                           VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                           texture.Image,
                                           texture.ImageMemory);
      !result) {
    device.DestroyBuffer(staging);
    return std::unexpected(TextureError{.Code = TextureError::ErrorCode::ImageUtilFailure, .Cause = result.error()});
  }

  // Staging buffer to texture image.
  if (auto result = imageutil::TransitionImageLayout(cmd,
                                                     texture.Image,
                                                     VK_FORMAT_R8G8B8A8_SRGB,
                                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
      !result) {
    device.DestroyBuffer(staging);
    return std::unexpected(TextureError{.Code = TextureError::ErrorCode::ImageUtilFailure, .Cause = result.error()});
  }

  copyBufferToImage_(staging.Buffer, texture.Image, width, height);

  if (auto result = imageutil::TransitionImageLayout(cmd,
                                                     texture.Image,
                                                     VK_FORMAT_R8G8B8A8_SRGB,
                                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      !result) {
    device.DestroyBuffer(staging);
    return std::unexpected(TextureError{.Code = TextureError::ErrorCode::ImageUtilFailure, .Cause = result.error()});
  }

  device.DestroyBuffer(staging);

  // Create the Imageview.
  if (auto result = createTextureImageView_(texture.Image, VK_FORMAT_R8G8B8A8_SRGB); result) {
    texture.ImageView = *result;
  } else {
    return std::unexpected(result.error());
  }

  // Create the Sampler.
  if (auto result = createSampler_(); !result) {
    return std::unexpected(result.error());
  } else {
    texture.Sampler = *result;
  }

  // Store the texture.
  textures_.push_back(texture);
  return textures_.size() - 1;
}

TextureGPU& TextureAllocator::Get(const uint32_t textureID) noexcept {
  assert(textureID < textures_.size());
  return textures_[textureID];
}

void TextureAllocator::copyBufferToImage_(VkBuffer buffer,
                                          VkImage image,
                                          const uint32_t width,
                                          const uint32_t height) const noexcept {
  const auto& cmd = context_.GetCommand();

  VkCommandBuffer commandBuffer = cmd.BeginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  cmd.EndSingleTimeCommands(commandBuffer);
}

std::expected<VkImageView, TextureError> TextureAllocator::createTextureImageView_(VkImage image,
                                                                                   VkFormat format) const {
  auto result = imageutil::CreateImageView(context_.GetDevice(), image, format, VK_IMAGE_ASPECT_COLOR_BIT);
  if (!result)
    return std::unexpected(TextureError{.Code = TextureError::ErrorCode::ImageUtilFailure, .Cause = result.error()});
  return *result;
}

std::expected<VkSampler, TextureError> TextureAllocator::createSampler_() const {
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

} // namespace engine::graphics::vulkan