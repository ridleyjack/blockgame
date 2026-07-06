#include "BlockHighlighter.hpp"

#include "Engine/Assets/ImageLoader.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/PipelineCreateInfo.hpp"
#include "Engine/Graphics/SubmitInfo.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <span>

namespace {
constexpr glm::vec3 HighlightColor{1.0f, 0.9f, 0.2f};

gfx::Mesh CreateBlockHighlightMesh() {
  constexpr float t = 0.035f;
  constexpr float e = 0.002f;

  constexpr float lo = -e;
  constexpr float hi = 1.0f + e;
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

BlockHighlighter::BlockHighlighter(vlk::Renderer& renderer) : renderer_{renderer} {
  auto shaderDataType = gfx::MakeShaderDataType<glm::mat4>();
  pipeline_ = renderer.CreatePipeline(gfx::PipelineCreateInfo{
      .Kind = gfx::PipelineKind::SolidGeometry,
      .VertexShaderFile = "Shaders/highlight.vert.spv",
      .FragmentShaderFile = "Shaders/highlight.frag.spv",
      .ShaderDataSlots = {shaderDataType},
  });

  mesh_ = renderer.CreateMesh(CreateBlockHighlightMesh());
  shaderDataHandle_ = renderer.CreateShaderData<glm::mat4>();
}
BlockHighlighter::~BlockHighlighter() {
  renderer_.DeleteMesh(mesh_);
  renderer_.DeletePipeline(pipeline_);
  renderer_.DeleteShaderData(shaderDataHandle_);
}

void BlockHighlighter::Upload() const {
  auto data = glm::translate(glm::mat4{1.0f}, glm::vec3{worldBlockPos_.X, worldBlockPos_.Y, worldBlockPos_.Z});
  renderer_.SetShaderData(shaderDataHandle_, data);
}

void BlockHighlighter::Draw() const {
  gfx::ShaderDataBinding binding = gfx::BindShaderData(shaderDataHandle_);
  renderer_.Submit({
      .Pipeline = pipeline_,
      .Mesh = mesh_,
      .ShaderData = std::span{&binding, 1},
  });
}

void BlockHighlighter::SetPosition(const math::Vec3Int worldBlockPos) {
  worldBlockPos_ = worldBlockPos;
}
