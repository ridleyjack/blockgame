#pragma once

#include "Engine/ILayer.hpp"
#include "Engine/IEvent.hpp"
#include "Engine/Renderer/VulkanRenderer.hpp"

class GameLayer : public Engine::ILayer {
public:
  explicit GameLayer(Engine::Renderer& renderer);
  ~GameLayer();

  void OnEvent(Engine::IEvent& event) override;

  void OnUpdate(float deltaTime) override;

  void OnRender() override;

private:
  Engine::Renderer& renderer_;
};
