#include "BlockHighlighter.hpp"

#include "Engine/Assets/ImageLoader.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/PipelineCreateInfo.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr glm::vec3 HighlightColor{1.0f, 0.9f, 0.2f};

gfx::Mesh CreateBlockHighlightMesh() {
  constexpr float t = 0.035f;
  constexpr float e = 0.002f;

  constexpr float lo = -0.5f - e;
  constexpr float hi = 0.5f + e;
  constexpr float stripScale = t / (hi - lo);

  gfx::Mesh mesh{};
  const auto addQuad = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
    const auto base = static_cast<std::uint32_t>(mesh.Vertices.size());

    mesh.Vertices.insert(mesh.Vertices.end(),
                         {
                             {a, HighlightColor},
                             {b, HighlightColor},
                             {c, HighlightColor},
                             {d, HighlightColor},
    });

    // Emit both windings so flat outlines stay visible with back-face culling enabled.
    mesh.Indices.insert(mesh.Indices.end(),
                        {
                            base + 0,
                            base + 1,
                            base + 2,
                            base + 2,
                            base + 3,
                            base + 0,
                            base + 0,
                            base + 3,
                            base + 2,
                            base + 2,
                            base + 1,
                            base + 0,
                        });
  };

  const auto addFaceOutline = [&](glm::vec3 bottomLeft, glm::vec3 bottomRight, glm::vec3 topRight, glm::vec3 topLeft) {
    const glm::vec3 horizontalStrip = (bottomRight - bottomLeft) * stripScale;
    const glm::vec3 verticalStrip = (topLeft - bottomLeft) * stripScale;

    addQuad(bottomLeft, bottomRight, bottomRight + verticalStrip, bottomLeft + verticalStrip);
    addQuad(topLeft - verticalStrip, topRight - verticalStrip, topRight, topLeft);
    addQuad(bottomLeft, bottomLeft + horizontalStrip, topLeft + horizontalStrip, topLeft);
    addQuad(bottomRight - horizontalStrip, bottomRight, topRight, topRight - horizontalStrip);
  };

  addFaceOutline({lo, lo, lo}, {hi, lo, lo}, {hi, hi, lo}, {lo, hi, lo});
  addFaceOutline({hi, lo, hi}, {lo, lo, hi}, {lo, hi, hi}, {hi, hi, hi});
  addFaceOutline({lo, lo, hi}, {lo, lo, lo}, {lo, hi, lo}, {lo, hi, hi});
  addFaceOutline({hi, lo, lo}, {hi, lo, hi}, {hi, hi, hi}, {hi, hi, lo});
  addFaceOutline({lo, hi, lo}, {hi, hi, lo}, {hi, hi, hi}, {lo, hi, hi});
  addFaceOutline({lo, lo, hi}, {hi, lo, hi}, {hi, lo, lo}, {lo, lo, lo});

  return mesh;
}

} // namespace

BlockHighlighter::BlockHighlighter(vlk::Renderer& renderer) {
  const gfx::PipelineHandle pipeline =
      renderer.CreatePipeline(gfx::PipelineCreateInfo{.VertexShaderFile = "Shaders/highlight.vert.spv",
                                                      .FragmentShaderFile = "Shaders/highlight.frag.spv"});

  // Create a dummy texture.
  auto image = engine::assets::LoadImage("Textures/Tiles/dirt.png");
  if (!image) {
    throw std::runtime_error{"Highlight failed to load dummy texture"};
  }
  auto texture = renderer.CreateTexture(image->Pixels, image->Width, image->Height);

  renderItem_.Pipeline = pipeline;
  renderItem_.Material = renderer.CreateMaterial(texture);
  gfx::Mesh meshData = CreateBlockHighlightMesh();
  mesh_ = renderer.CreateMesh(meshData);
}

const RenderItem& BlockHighlighter::GetRenderItem() const noexcept {
  return renderItem_;
}

gfx::MeshHandle BlockHighlighter::GetMesh() const noexcept {
  return mesh_;
}

void BlockHighlighter::SetPosition(const math::Vec3Int worldBlockPos) {
  renderItem_.PushConstants.Model =
      glm::translate(glm::mat4{1.0f}, glm::vec3{worldBlockPos.X, worldBlockPos.Y, worldBlockPos.Z});
}
