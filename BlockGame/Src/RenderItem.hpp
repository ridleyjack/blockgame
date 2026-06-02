#pragma once

#include "Engine/Graphics/Handles.hpp"
#include "Engine/Graphics/ObjectPushConstants.hpp"

namespace gfx = engine::graphics;

struct RenderItem {
  gfx::PipelineHandle Pipeline{};
  gfx::MaterialHandle Material{};
  gfx::ObjectPushConstants PushConstants{};
};
