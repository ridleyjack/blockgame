#pragma once

#include "Engine/ILayer.hpp"
#include "Engine/Events/IEventHandler.hpp"
#include "Engine/Graphics/Handles.hpp"

namespace engine {
class Application;

namespace events {
struct KeyPressedEvent;
struct KeyReleasedEvent;
} // namespace events
} // namespace engine

class GameLayer : public engine::ILayer, public engine::events::IKeyEventHandler {
public:
  explicit GameLayer(engine::Application& application);

  // void OnEvent(engine::Event& event) override;

  void OnUpdate(float deltaTime) override;

  void OnRender() override;

  void OnKeyReleased(const engine::events::KeyReleasedEvent& event) override;
  void OnKeyPressed(const engine::events::KeyPressedEvent& event) override;

private:
  engine::Application& application_;

  engine::graphics::MeshHandle myMesh_{};
};
