#pragma once

#include "RenderItem.hpp"

#include "Engine/Math/Vec3Int.hpp"

namespace engine::graphics::vulkan {
class Renderer;
}

namespace vlk = engine::graphics::vulkan;
namespace math = engine::math;

class BlockHighlighter {
public:
  BlockHighlighter(vlk::Renderer& renderer);

  const RenderItem& GetRenderItem() const noexcept;
  gfx::MeshHandle GetMesh() const noexcept;

  void SetPosition(math::Vec3Int worldBlockPos);

private:
  RenderItem renderItem_{};
  gfx::MeshHandle mesh_{};
};
