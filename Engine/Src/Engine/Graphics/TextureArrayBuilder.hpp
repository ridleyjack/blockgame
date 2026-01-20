#pragma once

#include <expected>
#include <span>

namespace engine::graphics {
struct TextureHandle;

namespace vulkan {
class TextureAllocator;
struct TextureError;
} // namespace vulkan

class TextureArrayBuilder {
public:
  explicit TextureArrayBuilder(vulkan::TextureAllocator& allocator);

  TextureArrayBuilder(const TextureArrayBuilder& Other) = delete;
  TextureArrayBuilder& operator=(const TextureArrayBuilder& Other) = delete;

  TextureArrayBuilder(TextureArrayBuilder&&) noexcept = default;

  // Upload to image data to the next layer of the texture array. Assumes number of layers is not exceeded,
  // enforced via assert while debugging.
  void Upload(std::span<const std::byte> pixels) const noexcept;

  // Finalize the array. Must be done before the building of another TextureArray is started.
  std::expected<TextureHandle, vulkan::TextureError> Finalize() const noexcept;

private:
  vulkan::TextureAllocator& allocator_;
};

} // namespace engine::graphics