#include "Application.hpp"

#include "GFX/Vulkan/VulkanRenderer.hpp"

#include <algorithm>
#include <cassert>

namespace engine {

// ==============================
// Public Methods
// ==============================

Application::Application(const ApplicationConfig& config) : config_(config) {
  glfwInit();

  if (config_.Window.Title.empty()) {
    config_.Window.Title = config.Name;
  }

  window_ = std::make_unique<Window>(config.Window);
  window_->Create();

  renderer_ = std::make_unique<gfx::vulkan::Renderer>(window_->GetHandle());
  renderer_->Init();
}

Application::~Application() {
  window_.reset();
}

void Application::Run() {
  running_ = true;
  float lastTime = GetTime();

  while (running_) {
    glfwPollEvents();

    if (window_->ShouldClose()) {
      Stop();
      break;
    }

    const float currentTime = GetTime();
    const float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    for (ILayer* layer : layerStack_)
      layer->OnUpdate(deltaTime);

    for (ILayer* layer : layerStack_)
      layer->OnRender();
  }
  renderer_->WaitUntilIdle();
}

void Application::Stop() {
  running_ = false;
}

void Application::PushLayer(ILayer* layer) {
  assert(layer != nullptr);
  layerStack_.push_back(layer);
}

void Application::RemoveLayer(const ILayer* layer) {
  const auto it = std::find(layerStack_.begin(), layerStack_.end(), layer);
  if (it != layerStack_.end())
    return;
  layerStack_.erase(it);
}

float Application::GetTime() {
  return static_cast<float>(glfwGetTime());
}
Window& Application::GetWindow() const {
  return *window_;
}
gfx::vulkan::Renderer& Application::GetRenderer() const {
  return *renderer_;
}

// ==============================
// Private Methods
// ==============================

} // namespace engine