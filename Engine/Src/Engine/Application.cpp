#include "Application.hpp"

#include "ILayer.hpp"

#include "Events/IEventHandler.hpp"

#include "Graphics/Vulkan/Renderer.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

namespace engine {

Application::Application(const ApplicationConfig& config) : config_(config) {
  glfwInit();

  if (config_.Window.Title.empty()) {
    config_.Window.Title = config.Name;
  }

  window_ = std::make_unique<Window>(config.Window, *this);
  window_->Create();

  renderer_ = std::make_unique<graphics::vulkan::Renderer>(window_->GetHandle());
}

Application::~Application() {
  window_.reset();
  glfwTerminate();
}

void Application::Run() {
  running_ = true;
  float lastTime = GetTime();

  while (running_) {
    glfwPollEvents();

    if (window_->ShouldClose()) {
      Stop();
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
  if (it == layerStack_.end())
    return;
  layerStack_.erase(it);
}

float Application::GetTime() {
  return static_cast<float>(glfwGetTime());
}

Window& Application::GetWindow() const {
  return *window_;
}

graphics::vulkan::Renderer& Application::GetRenderer() const {
  return *renderer_;
}

void Application::RaiseEvent(const events::Event& event) {
  for (const auto& layer : std::views::reverse(layerStack_)) {
    if (auto* handler = dynamic_cast<events::IKeyEventHandler*>(layer)) {
      std::visit(events::KeyEventDispatch{.Handler = *handler}, event);
    }
  }
}

} // namespace engine