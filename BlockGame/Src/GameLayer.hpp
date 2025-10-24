#pragma once

#include "Engine/ILayer.hpp"
#include "Engine/IEvent.hpp"
#include "Engine/GFX/Vulkan/VulkanRenderer.hpp"

class GameLayer : public engine::ILayer {
public:
  explicit GameLayer(engine::gfx::vulkan::Renderer& renderer);
  ~GameLayer();

  void OnEvent(engine::IEvent& event) override;

  void OnUpdate(float deltaTime) override;

  void OnRender() override;

private:
  engine::gfx::vulkan::Renderer& renderer_;
};
