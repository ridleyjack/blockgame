#include "GameLayer.hpp"

// ==============================
// Public Methods
// ==============================

GameLayer::GameLayer(engine::gfx::vulkan::Renderer& renderer) : renderer_(renderer) {}

GameLayer::~GameLayer() {}

void GameLayer::OnEvent(engine::IEvent& event) {};

void GameLayer::OnUpdate(float deltaTime) {};

void GameLayer::OnRender() {
  renderer_.DrawFrame();
};

// ==============================
// Private Methods
// ==============================
