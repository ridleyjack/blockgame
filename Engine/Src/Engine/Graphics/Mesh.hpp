#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace engine::graphics {
struct Vertex {
  glm::vec3 Position{};
  glm::vec3 Color{};
  glm::vec2 TexCoord{};
  std::uint32_t TextureIndex{};
};

struct Mesh {
  std::vector<Vertex> Vertices{};
  std::vector<std::uint32_t> Indices{};
};
} // namespace engine::graphics