#pragma once

#include "Camera.hpp"
#include "ChunkMeshes.hpp"
#include "Map.hpp"
#include "Engine/ILayer.hpp"
#include "Engine/Events/IEventHandler.hpp"

namespace engine {
class Application;

namespace graphics {
struct Vertex;
}

namespace events {
struct KeyPressedEvent;
struct KeyReleasedEvent;
} // namespace events
} // namespace engine

namespace gfx = engine::graphics;

struct Movements {
  bool Forward{};
  bool Backward{};
  bool Left{};
  bool Right{};
};

struct KeyInput {
  Movements Movement{};
  bool Exit{};
};

class GameLayer : public engine::ILayer,
                  public engine::events::IKeyEventHandler,
                  public engine::events::IMouseEventHandler {
public:
  explicit GameLayer(engine::Application& application);

  void OnUpdate(float deltaTime) override;

  void OnRender() override;

  void OnKeyReleased(const engine::events::KeyReleasedEvent& event) override;
  void OnKeyPressed(const engine::events::KeyPressedEvent& event) override;

  void OnMouseMoved(const engine::events::MouseMovedEvent& event) override;

private:
  engine::Application& application_;

  Camera camera_{};
  KeyInput input_{};

  Map map_;
  ChunkMeshes mapMeshes_;

  struct RenderItem {
    gfx::RenderPassHandle RenderPass{};
    gfx::PipelineHandle Pipeline{};
    gfx::MaterialHandle Material{};
  };
  RenderItem renderItem_;

  bool firstMouse_{true};
  float lastX_{}, lastY_{};

  void handleKeyInput(int keycode, bool state);
};
