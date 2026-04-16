#pragma once

#include "Engine/Graphics/Handles.hpp"

#include <array>
#include <cstdint>

namespace engine::graphics::vulkan {
class Renderer;
}

namespace gfx = engine::graphics;
namespace vlk = gfx::vulkan;

// Temporary: block texture enum values are used as texture-array layer indices.
// This couples BlockTexture to the current texture array layout.
enum class BlockTexture : std::uint8_t {
  Dirt = 0,
  DirtGrass,
  GrassTop,
  DirtSand,
  DirtSnow,
  Sand,
  Snow,
  Ice,
  Stone,
  StoneDirt,
  StoneGrass,
  StoneSnow,
};

struct RenderItem {
  gfx::RenderPassHandle RenderPass{};
  gfx::PipelineHandle Pipeline{};
  gfx::MaterialHandle Material{};
};

class TextureRegistry {
public:
  explicit TextureRegistry(vlk::Renderer& renderer);

  const RenderItem& GetRenderItem() const noexcept;

private:
  RenderItem renderItem_;
};
