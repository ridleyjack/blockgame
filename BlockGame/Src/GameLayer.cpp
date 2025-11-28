#include "GameLayer.hpp"

#include "Engine/Application.hpp"
#include "Engine/GFX/Vulkan/VulkanRenderer.hpp"

// ==============================
// Public Methods
// ==============================

GameLayer::GameLayer(engine::Application& application) : application_(application) {}

GameLayer::~GameLayer() {}

void GameLayer::OnEvent(engine::IEvent& event) {};

void GameLayer::OnUpdate(float deltaTime) {};

void GameLayer::OnRender() {
  application_.GetRenderer().DrawFrame();
};

// ==============================
// Private Methods
// ==============================
