#include "Crosshair.hpp"

#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/PipelineCreateInfo.hpp"
#include "Engine/Graphics/SubmitInfo.hpp"
#include "Engine/Graphics/Vulkan/Renderer.hpp"

namespace {
constexpr glm::vec3 CrosshairColor{1.0f, 1.0f, 1.0f};

gfx::Mesh CreateCrosshairMesh() {
  constexpr float armLength = 0.035f;
  constexpr float centerGap = 0.008f;
  constexpr float halfThickness = 0.0025f;

  gfx::Mesh mesh{};
  const auto addQuad = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
    const auto base = static_cast<std::uint32_t>(mesh.Vertices.size());

    mesh.Vertices.insert(mesh.Vertices.end(),
                         {
                             {a, CrosshairColor},
                             {b, CrosshairColor},
                             {c, CrosshairColor},
                             {d, CrosshairColor},
    });

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

  addQuad({-armLength, -halfThickness, 0.0f},
          {-centerGap, -halfThickness, 0.0f},
          {-centerGap, halfThickness, 0.0f},
          {-armLength, halfThickness, 0.0f});
  addQuad({centerGap, -halfThickness, 0.0f},
          {armLength, -halfThickness, 0.0f},
          {armLength, halfThickness, 0.0f},
          {centerGap, halfThickness, 0.0f});
  addQuad({-halfThickness, -armLength, 0.0f},
          {halfThickness, -armLength, 0.0f},
          {halfThickness, -centerGap, 0.0f},
          {-halfThickness, -centerGap, 0.0f});
  addQuad({-halfThickness, centerGap, 0.0f},
          {halfThickness, centerGap, 0.0f},
          {halfThickness, armLength, 0.0f},
          {-halfThickness, armLength, 0.0f});

  return mesh;
}
} // namespace

Crosshair::Crosshair(vlk::Renderer& renderer) : renderer_(renderer) {
  pipeline_ = renderer.CreatePipeline(gfx::PipelineCreateInfo{.Kind = gfx::PipelineKind::SolidGeometry,
                                                              .VertexShaderFile = "Shaders/crosshair.vert.spv",
                                                              .FragmentShaderFile = "Shaders/crosshair.frag.spv"});
  mesh_ = renderer.CreateMesh(CreateCrosshairMesh());
}

Crosshair::~Crosshair() {
  renderer_.DeleteMesh(mesh_);
  renderer_.DeletePipeline(pipeline_);
}

void Crosshair::Draw() const {
  renderer_.Submit(gfx::SubmitInfo{.Pipeline = pipeline_, .Mesh = mesh_});
}
