#include "TextureArrayBuilder.hpp"

#include "Handles.hpp"
#include "Vulkan/TextureAllocator.hpp"

namespace engine::graphics {
TextureArrayBuilder::TextureArrayBuilder(vulkan::TextureAllocator& allocator) : allocator_(allocator) {}

void TextureArrayBuilder::Upload(const std::span<const std::byte> pixels) const noexcept {
  allocator_.UploadLayer(pixels);
}

std::expected<TextureHandle, vulkan::TextureError> TextureArrayBuilder::Finalize() const noexcept {
  auto result = allocator_.FinishArray();
  if (!result)
    return std::unexpected(result.error());
  return TextureHandle{.TextureID = *result};
}
} // namespace engine::graphics
