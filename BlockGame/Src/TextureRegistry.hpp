#pragma once

#include "BlockTypes.hpp"

#include "Engine/Graphics/Handles.hpp"

#include <array>
#include <string_view>
#include <cassert>

namespace engine::graphics::vulkan {
class Renderer;
}

namespace gfx = engine::graphics;
namespace vlk = gfx::vulkan;

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
  struct BlockTextureResource {
    BlockTexture TextureID;
    std::string_view Path;
  };

  static constexpr std::array<BlockTextureResource, static_cast<std::size_t>(BlockTexture::Count)> blockTextures_{
      {
       {BlockTexture::Dirt, "Textures/Tiles/dirt.png"},
       {BlockTexture::DirtGrass, "Textures/Tiles/dirt_grass.png"},
       {BlockTexture::GrassTop, "Textures/Tiles/grass_top.png"},
       {BlockTexture::DirtSand, "Textures/Tiles/dirt_sand.png"},
       {BlockTexture::DirtSnow, "Textures/Tiles/dirt_snow.png"},
       {BlockTexture::Sand, "Textures/Tiles/sand.png"},
       {BlockTexture::Snow, "Textures/Tiles/snow.png"},
       {BlockTexture::Ice, "Textures/Tiles/ice.png"},
       {BlockTexture::Stone, "Textures/Tiles/stone.png"},
       {BlockTexture::StoneDirt, "Textures/Tiles/stone_dirt.png"},
       {BlockTexture::StoneGrass, "Textures/Tiles/stone_grass.png"},
       {BlockTexture::StoneSnow, "Textures/Tiles/stone_snow.png"},
       }
  };

  RenderItem renderItem_;
};
