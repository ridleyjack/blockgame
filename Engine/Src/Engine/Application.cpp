#include "Application.hpp"

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
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    for (ILayer* layer : layerStack_)
      layer->OnUpdate(deltaTime);

    for (ILayer* layer : layerStack_)
      layer->OnRender();
  }
}

void Application::Stop() {
  running_ = false;
}

void Application::PushLayer(ILayer* layer) {
  assert(layer != nullptr);
  layerStack_.push_back(layer);
}

void Application::RemoveLayer(ILayer* layer) {
  auto it = std::find(layerStack_.begin(), layerStack_.end(), layer);
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

// ==============================
// Private Methods
// ==============================

} // namespace Engine