#include "Highlight.hpp"

#include "Engine/Assets/ImageLoader.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/PipelineCreateInfo.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr glm::vec3 HighlightColor{1.0f, 0.9f, 0.2f};

void AddQuad(gfx::Mesh& mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
  const auto base = static_cast<std::uint32_t>(mesh.Vertices.size());

  mesh.Vertices.insert(mesh.Vertices.end(),
                       {
                           {a, HighlightColor},
                           {b, HighlightColor},
                           {c, HighlightColor},
                           {d, HighlightColor},
                       });
  mesh.Indices.insert(mesh.Indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

void AddBox(gfx::Mesh& mesh, const glm::vec3 min, const glm::vec3 max) {
  const glm::vec3 leftBottomBack{min.x, min.y, min.z};
  const glm::vec3 rightBottomBack{max.x, min.y, min.z};
  const glm::vec3 rightTopBack{max.x, max.y, min.z};
  const glm::vec3 leftTopBack{min.x, max.y, min.z};
  const glm::vec3 leftBottomFront{min.x, min.y, max.z};
  const glm::vec3 rightBottomFront{max.x, min.y, max.z};
  const glm::vec3 rightTopFront{max.x, max.y, max.z};
  const glm::vec3 leftTopFront{min.x, max.y, max.z};

  AddQuad(mesh, leftBottomBack, leftTopBack, rightTopBack, rightBottomBack);       // Back
  AddQuad(mesh, leftBottomFront, rightBottomFront, rightTopFront, leftTopFront);  // Front
  AddQuad(mesh, leftBottomBack, leftBottomFront, leftTopFront, leftTopBack);      // Left
  AddQuad(mesh, rightBottomBack, rightTopBack, rightTopFront, rightBottomFront);  // Right
  AddQuad(mesh, leftBottomBack, rightBottomBack, rightBottomFront, leftBottomFront);  // Bottom
  AddQuad(mesh, leftTopBack, leftTopFront, rightTopFront, rightTopBack);          // Top
}

gfx::Mesh CreateBlockHighlightMesh() {
  constexpr float t = 0.035f;
  constexpr float e = 0.002f;

  const float lo = -e;
  const float hi = 1.0f + e;

  gfx::Mesh mesh;

  // Bottom square, y = 0
  AddBox(mesh, {lo, lo, lo}, {hi, t, t});
  AddBox(mesh, {lo, lo, hi - t}, {hi, t, hi});
  AddBox(mesh, {lo, lo, lo}, {t, t, hi});
  AddBox(mesh, {hi - t, lo, lo}, {hi, t, hi});

  // Top square, y = 1
  AddBox(mesh, {lo, hi - t, lo}, {hi, hi, t});
  AddBox(mesh, {lo, hi - t, hi - t}, {hi, hi, hi});
  AddBox(mesh, {lo, hi - t, lo}, {t, hi, hi});
  AddBox(mesh, {hi - t, hi - t, lo}, {hi, hi, hi});

  // Vertical edges
  AddBox(mesh, {lo, lo, lo}, {t, hi, t});
  AddBox(mesh, {hi - t, lo, lo}, {hi, hi, t});
  AddBox(mesh, {lo, lo, hi - t}, {t, hi, hi});
  AddBox(mesh, {hi - t, lo, hi - t}, {hi, hi, hi});

  return mesh;
}
} // namespace

Highlight::Highlight(vlk::Renderer& renderer) {
  const gfx::PipelineHandle pipeline =
      renderer.CreatePipeline(gfx::PipelineCreateInfo{.VertexShaderFile = "Shaders/highlight.vert.spv",
                                                      .FragmentShaderFile = "Shaders/highlight.frag.spv"});

  // Create a dummy texture.
  auto image = engine::assets::LoadImage("Textures/Tiles/dirt.png");
  if (!image) {
    throw std::runtime_error{"Highlight failed to load dummy texture"};
  }
  auto texture = renderer.CreateTexture(image->Pixels, image->Width, image->Height);
  if (!texture) {
    throw std::runtime_error{"Highlight failed to create dummy texture"};
  }

  renderItem_.Pipeline = pipeline;
  renderItem_.Material = renderer.CreateMaterial(*texture);
  renderItem_.PushConstants.Model =
      glm::translate(glm::mat4{1.0f}, glm::vec3{16.0f * 1.5f, 16.0f * 3.0f, 16.0f * 1.5f});
  gfx::Mesh meshData = CreateBlockHighlightMesh();
  mesh_ = renderer.CreateMesh(meshData);
}

const RenderItem& Highlight::GetRenderItem() const noexcept {
  return renderItem_;
}

gfx::MeshHandle Highlight::GetMesh() const noexcept {
  return mesh_;
}
