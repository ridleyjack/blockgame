#include "TextureArrayBuilder.hpp"

#include "Handles.hpp"
#include "Vulkan/TextureAllocator.hpp"

namespace engine::graphics {
TextureArrayBuilder::TextureArrayBuilder(vulkan::TextureAllocator& allocator) : allocator_(allocator) {}

void TextureArrayBuilder::Upload(const std::span<const std::byte> pixels) const noexcept {
  allocator_.UploadLayer(pixels);
}

TextureHandle TextureArrayBuilder::Finalize() const noexcept {
  return TextureHandle{.TextureID = allocator_.FinishArray()};
}
} // namespace engine::graphics
