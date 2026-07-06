#pragma once

#include "RenderItem.hpp"

namespace engine::graphics::vulkan {
class Renderer;
}

namespace vlk = engine::graphics::vulkan;

class Crosshair {
public:
  explicit Crosshair(vlk::Renderer& renderer);
  ~Crosshair();

  Crosshair(const Crosshair&) = delete;
  Crosshair& operator=(const Crosshair&) = delete;

  void Draw() const;

private:
  vlk::Renderer& renderer_;
  gfx::PipelineHandle pipeline_;

  gfx::MeshHandle mesh_{};
};
