#pragma once

#include "Camera.hpp"
#include "MapMesh.hpp"
#include "Engine/ILayer.hpp"
#include "Engine/Events/IEventHandler.hpp"
#include "Engine/Graphics/Handles.hpp"
#include "Engine/Graphics/Mesh.hpp"

#include <vector>

namespace engine {
class Application;

namespace graphics {
class Vertex;
}

namespace events {
struct KeyPressedEvent;
struct KeyReleasedEvent;
} // namespace events

} // namespace engine

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
  Camera camera_{};
  KeyInput input_{};

  MapMesh myMesh_{};

  bool firstMouse_{true};
  float lastX_{}, lastY_{};

  engine::Application& application_;
  void handleKeyInput(int keycode, bool state);
};
