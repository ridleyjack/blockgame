#pragma once

#include "Engine/Graphics/Handles.hpp"
#include "Engine/Math/Vec3Int.hpp"

#include "glm/fwd.hpp"

namespace engine::graphics::vulkan {
class Renderer;
}

namespace gfx = engine::graphics;
namespace vlk = engine::graphics::vulkan;
namespace math = engine::math;

class BlockHighlighter {
public:
  explicit BlockHighlighter(vlk::Renderer& renderer);
  ~BlockHighlighter();

  BlockHighlighter(const BlockHighlighter&) = delete;
  BlockHighlighter& operator=(const BlockHighlighter&) = delete;

  void Upload() const;
  void Draw() const;

  void SetPosition(math::Vec3Int worldBlockPos);

private:
  vlk::Renderer& renderer_;

  gfx::PipelineHandle pipeline_{};
  gfx::MeshHandle mesh_{};
  math::Vec3Int worldBlockPos_{};

  gfx::ShaderDataHandle<glm::mat4> shaderDataHandle_{};
};
