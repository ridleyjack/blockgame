#pragma once

#include "Engine/ILayer.hpp"
#include "Engine/IEvent.hpp"

namespace engine {
class Application;
}

class GameLayer : public engine::ILayer {
public:
  explicit GameLayer(engine::Application& application);
  ~GameLayer();

  void OnEvent(engine::IEvent& event) override;

  void OnUpdate(float deltaTime) override;

  void OnRender() override;

private:
  engine::Application& application_;
};
