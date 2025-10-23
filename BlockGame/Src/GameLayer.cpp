#include "GameLayer.hpp"

#include <print>

// ==============================
// Public Methods
// ==============================

GameLayer::GameLayer(Engine::Renderer& renderer) : renderer_(renderer) {}

GameLayer::~GameLayer() {}

void GameLayer::OnEvent(Engine::IEvent& event) {};

void GameLayer::OnUpdate(float deltaTime) {};

void GameLayer::OnRender() {
  renderer_.DrawFrame();
};

// ==============================
// Private Methods
// ==============================
