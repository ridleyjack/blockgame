#pragma once

#include "RenderItem.hpp"

namespace engine::graphics::vulkan {
class Renderer;
}

namespace vlk = engine::graphics::vulkan;

class Highlight {
public:
  Highlight(vlk::Renderer& renderer);

  const RenderItem& GetRenderItem() const noexcept;
  gfx::MeshHandle GetMesh() const noexcept;

private:
  RenderItem renderItem_{};
  gfx::MeshHandle mesh_{};
};
