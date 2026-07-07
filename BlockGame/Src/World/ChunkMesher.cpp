#include "ChunkMesher.hpp"

#include "BlockRegistry.hpp"
#include "WorldStore.hpp"
#include "Containers/Grid3D.hpp"

#include "Engine/Graphics/Vulkan/Renderer.hpp"

#include <cstddef>
#include <thread>

ChunkMesher::ChunkMesher(vlk::Renderer& renderer, WorldStore& worldStore, BlockRegistry& blockRegistry)
    : renderer_(renderer),
      worldStore_(worldStore),
      meshes_(WorldStore::WorldDepth, WorldStore::WorldHeight, WorldStore::WorldWidth, {}),
      blockRegistry_(blockRegistry) {

  std::uint32_t threadNum = std::thread::hardware_concurrency();
  threadNum = threadNum < 2 ? 1 : threadNum - 1;
  startWorkers_(threadNum);
}

ChunkMesher::~ChunkMesher() {
  stopWorkers_();
}

std::optional<gfx::MeshHandle> ChunkMesher::RenderableMesh(const math::Vec3Int mapCoord) {
  assert(mapCoord.Z < meshes_.Depth() && mapCoord.Y < meshes_.Height() && mapCoord.X < meshes_.Width());

  auto& mesh = meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X];
  if (mesh.PendingHandle && renderer_.IsMeshReady(*mesh.PendingHandle)) {
    if (mesh.VisibleHandle)
      renderer_.DeleteMesh(*mesh.VisibleHandle);
    mesh.VisibleHandle = mesh.PendingHandle;
    mesh.PendingHandle = std::nullopt;
  }

  return meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X].VisibleHandle;
}

void ChunkMesher::RequestLoad(const math::Vec3Int mapCoord) {
  ChunkMeshSlot& meshSlot = meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X];
  if (meshSlot.Status == ChunkMeshStatus::Building || meshSlot.Status == ChunkMeshStatus::Uploaded)
    return; // Must request deletion of mesh before building it again.
  enqueueBuild_(mapCoord);
}

void ChunkMesher::RequestRebuild(const math::Vec3Int mapCoord) {
  enqueueBuild_(mapCoord);
}

void ChunkMesher::RequestUnload(const math::Vec3Int mapCoord) {
  auto& meshSlot = meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X];

  if (meshSlot.VisibleHandle)
    renderer_.DeleteMesh(*meshSlot.VisibleHandle);
  if (meshSlot.PendingHandle)
    renderer_.DeleteMesh(*meshSlot.PendingHandle);

  meshSlot.VisibleHandle = std::nullopt;
  meshSlot.PendingHandle = std::nullopt;
  meshSlot.Wanted = false;
  meshSlot.Status = ChunkMeshStatus::Unloaded;
}

ChunkMeshStatus ChunkMesher::ChunkStatus(const math::Vec3Int mapCoord) {
  return meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X].Status;
}

void ChunkMesher::Update() {
  while (auto opt = resultQueue_.TryPop()) {
    auto& result = *opt;

    auto& meshSlot = meshes_[result.Coord.Z, result.Coord.Y, result.Coord.X];
    if (meshSlot.Generation != result.Generation || !meshSlot.Wanted)
      continue; // Discard outdated or un-needed result.

    if (result.Status == ChunkMeshStatus::MissingDependencies) {
      meshSlot.Status = ChunkMeshStatus::MissingDependencies;
      continue; // The caller handles missing dependencies.
    }

    meshSlot.Status = ChunkMeshStatus::Uploaded;

    // Promote pending to visible if done or delete it.
    if (meshSlot.PendingHandle) {
      if (renderer_.IsMeshReady(*meshSlot.PendingHandle)) {
        if (meshSlot.VisibleHandle)
          renderer_.DeleteMesh(*meshSlot.VisibleHandle);
        meshSlot.VisibleHandle = meshSlot.PendingHandle;
      } else {
        renderer_.DeleteMesh(*meshSlot.PendingHandle);
      }
      meshSlot.PendingHandle = std::nullopt;
    }

    // Only meshes with vertices are uploaded to the GPU.
    if (result.Mesh.HasVertices()) {
      const auto handle = renderer_.CreateMesh({.Vertices = result.Mesh.Vertices, .Indices = result.Mesh.Indices});
      meshSlot.PendingHandle = handle;
    } else {
      if (meshSlot.VisibleHandle)
        renderer_.DeleteMesh(*meshSlot.VisibleHandle);
      if (meshSlot.PendingHandle)
        renderer_.DeleteMesh(*meshSlot.PendingHandle);
      meshSlot.VisibleHandle = std::nullopt;
      meshSlot.PendingHandle = std::nullopt;
    }
  }
}

void ChunkMesher::startWorkers_(const std::uint32_t count) {
  stop_ = false;
  for (std::uint32_t i = 0; i < count; i++) {
    workers_.emplace_back(&ChunkMesher::workerLoop_, this);
  }
}

void ChunkMesher::stopWorkers_() {
  stop_.store(true);
  buildQueue_.NotifyAll();

  for (auto& t : workers_) {
    if (t.joinable()) {
      t.join();
    }
  }
  workers_.clear();
}

void ChunkMesher::workerLoop_() {
  while (true) {
    auto job = buildQueue_.WaitPop(stop_);
    if (!job) {
      return; // Stop requested.
    }

    ChunkBuildResult buildResult{.Coord = job->Coord, .Generation = job->Generation};
    const auto worldView = worldStore_.AcquireReadView();

    if (!buildDependenciesReady_(worldView, job->Coord)) {
      buildResult.Status = ChunkMeshStatus::MissingDependencies;
    } else {
      auto meshResult = buildChunk_(worldView, job->Coord);
      buildResult.Status = ChunkMeshStatus::Building; // Building includes uploading.
      buildResult.Mesh = std::move(meshResult);
    }

    resultQueue_.Push(buildResult);
  }
}

void ChunkMesher::enqueueBuild_(const math::Vec3Int mapCoord) {
  ChunkMeshSlot& meshSlot = meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X];

  meshSlot.Wanted = true;
  meshSlot.Generation++;
  meshSlot.Status = ChunkMeshStatus::Building;
  buildQueue_.Push({.Coord = mapCoord, .Generation = meshSlot.Generation});
}

std::array<math::Vec3Int, 27> ChunkMesher::GetRequiredChunks(const math::Vec3Int chunkCoord) const {
  std::array<math::Vec3Int, 27> chunks{};
  std::size_t index = 0;

  for (std::int32_t z = -1; z <= 1; z++) {
    for (std::int32_t y = -1; y <= 1; y++) {
      for (std::int32_t x = -1; x <= 1; x++) {
        chunks[index++] = {
            .X = chunkCoord.X + x,
            .Y = chunkCoord.Y + y,
            .Z = chunkCoord.Z + z,
        };
      }
    }
  }

  return chunks;
}

bool ChunkMesher::buildDependenciesReady_(const WorldStore::ReadView& worldView, const math::Vec3Int chunkCoord) const {
  for (const math::Vec3Int chunk : GetRequiredChunks(chunkCoord)) {
    if (chunk.X < 0 || chunk.X >= WorldStore::WorldWidth || chunk.Y < 0 || chunk.Y >= WorldStore::WorldHeight ||
        chunk.Z < 0 || chunk.Z >= WorldStore::WorldDepth) {
      continue;
    }

    if (worldView.GetChunk(chunk) == nullptr)
      return false;
  }

  return true;
}

ChunkMesh ChunkMesher::buildChunk_(const WorldStore::ReadView& worldView, const math::Vec3Int chunkCoord) {
  const auto chunk = worldView.GetChunk(chunkCoord);
  auto& blocks = chunk->Blocks;
  ChunkMesh mesh{};
  for (std::int32_t z = 0; z < blocks.Depth(); z++) {
    for (std::int32_t y = 0; y < blocks.Height(); y++) {
      for (std::int32_t x = 0; x < blocks.Width(); x++) {

        if (blocks[z, y, x] == 0)
          continue;

        auto getBlock = [&](const int deltaZ, const int deltaY, const int deltaX) -> std::uint32_t {
          math::Vec3Int worldCoord{.X = static_cast<std::int32_t>(chunkCoord.X * blocks.Width()) + x,
                                   .Y = static_cast<std::int32_t>(chunkCoord.Y * blocks.Height()) + y,
                                   .Z = static_cast<std::int32_t>(chunkCoord.Z * blocks.Depth()) + z};

          const int targetZ = z + deltaZ;
          const int targetY = y + deltaY;
          const int targetX = x + deltaX;

          if (targetZ >= 0 && targetZ < blocks.Depth() && targetY >= 0 && targetY < blocks.Height() && targetX >= 0 &&
              targetX < blocks.Width())
            return blocks[targetZ, targetY, targetX];

          worldCoord.Z += deltaZ;
          worldCoord.Y += deltaY;
          worldCoord.X += deltaX;

          if (worldCoord.Z < 0 || worldCoord.Z >= WorldStore::WorldDepth * Chunk::ChunkDepth || worldCoord.Y < 0 ||
              worldCoord.Y >= WorldStore::WorldHeight * Chunk::ChunkHeight || worldCoord.X < 0 ||
              worldCoord.X >= WorldStore::WorldWidth * Chunk::ChunkWidth)
            return 0;

          const Chunk* neighbour = worldView.GetChunk({static_cast<std::int32_t>(worldCoord.X / Chunk::ChunkWidth),
                                                       static_cast<std::int32_t>(worldCoord.Y / Chunk::ChunkHeight),
                                                       static_cast<std::int32_t>(worldCoord.Z / Chunk::ChunkDepth)});
          assert(neighbour != nullptr);
          return neighbour->Blocks[worldCoord.Z % Chunk::ChunkDepth,
                                   worldCoord.Y % Chunk::ChunkHeight,
                                   worldCoord.X % Chunk::ChunkWidth];
        };

        BlockFaces faces{};
        if (getBlock(-1, 0, 0) == 0)
          faces.Back = true;
        if (getBlock(+1, 0, 0) == 0)
          faces.Front = true;

        if (getBlock(0, -1, 0) == 0)
          faces.Bottom = true;
        if (getBlock(0, 1, 0) == 0)
          faces.Top = true;

        if (getBlock(0, 0, -1) == 0)
          faces.Left = true;
        if (getBlock(0, 0, 1) == 0)
          faces.Right = true;

        if (faces.NumEnabled() == 0)
          continue;

        auto calculateAO = [&](const math::Vec3Int normal, const math::Vec3Int sideA, const math::Vec3Int sideB) {
          const auto isSolid = [&](const math::Vec3Int offset) { return getBlock(offset.Z, offset.Y, offset.X) != 0; };

          const bool a = isSolid({.X = normal.X + sideA.X, .Y = normal.Y + sideA.Y, .Z = normal.Z + sideA.Z});
          const bool b = isSolid({.X = normal.X + sideB.X, .Y = normal.Y + sideB.Y, .Z = normal.Z + sideB.Z});
          const bool corner = isSolid({.X = normal.X + sideA.X + sideB.X,
                                       .Y = normal.Y + sideA.Y + sideB.Y,
                                       .Z = normal.Z + sideA.Z + sideB.Z});

          const int occlusion = a && b ? 3 : static_cast<int>(a) + static_cast<int>(b) + static_cast<int>(corner);
          constexpr float minLight = 0.45f;
          return minLight + (1.0f - minLight) * (3.0f - static_cast<float>(occlusion)) / 3.0f;
        };

        FaceAmbientOcclusion ambientOcclusion{};
        if (faces.Front)
          ambientOcclusion[Front] = {
              calculateAO({.X = 0, .Y = 0, .Z = 1}, {.X = -1, .Y = 0, .Z = 0}, {.X = 0, .Y = -1, .Z = 0}),
              calculateAO({.X = 0, .Y = 0, .Z = 1}, {.X = 1, .Y = 0, .Z = 0}, {.X = 0, .Y = -1, .Z = 0}),
              calculateAO({.X = 0, .Y = 0, .Z = 1}, {.X = 1, .Y = 0, .Z = 0}, {.X = 0, .Y = 1, .Z = 0}),
              calculateAO({.X = 0, .Y = 0, .Z = 1}, {.X = -1, .Y = 0, .Z = 0}, {.X = 0, .Y = 1, .Z = 0}),
          };
        if (faces.Back)
          ambientOcclusion[Back] = {
              calculateAO({.X = 0, .Y = 0, .Z = -1}, {.X = 1, .Y = 0, .Z = 0}, {.X = 0, .Y = -1, .Z = 0}),
              calculateAO({.X = 0, .Y = 0, .Z = -1}, {.X = -1, .Y = 0, .Z = 0}, {.X = 0, .Y = -1, .Z = 0}),
              calculateAO({.X = 0, .Y = 0, .Z = -1}, {.X = -1, .Y = 0, .Z = 0}, {.X = 0, .Y = 1, .Z = 0}),
              calculateAO({.X = 0, .Y = 0, .Z = -1}, {.X = 1, .Y = 0, .Z = 0}, {.X = 0, .Y = 1, .Z = 0}),
          };
        if (faces.Right)
          ambientOcclusion[Right] = {
              calculateAO({.X = 1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = 1}, {.X = 0, .Y = -1, .Z = 0}),
              calculateAO({.X = 1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = -1}, {.X = 0, .Y = -1, .Z = 0}),
              calculateAO({.X = 1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = -1}, {.X = 0, .Y = 1, .Z = 0}),
              calculateAO({.X = 1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = 1}, {.X = 0, .Y = 1, .Z = 0}),
          };
        if (faces.Left)
          ambientOcclusion[Left] = {
              calculateAO({.X = -1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = -1}, {.X = 0, .Y = -1, .Z = 0}),
              calculateAO({.X = -1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = 1}, {.X = 0, .Y = -1, .Z = 0}),
              calculateAO({.X = -1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = 1}, {.X = 0, .Y = 1, .Z = 0}),
              calculateAO({.X = -1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = -1}, {.X = 0, .Y = 1, .Z = 0}),
          };
        if (faces.Top)
          ambientOcclusion[Top] = {
              calculateAO({.X = 0, .Y = 1, .Z = 0}, {.X = -1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = 1}),
              calculateAO({.X = 0, .Y = 1, .Z = 0}, {.X = 1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = 1}),
              calculateAO({.X = 0, .Y = 1, .Z = 0}, {.X = 1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = -1}),
              calculateAO({.X = 0, .Y = 1, .Z = 0}, {.X = -1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = -1}),
          };
        if (faces.Bottom)
          ambientOcclusion[Bottom] = {
              calculateAO({.X = 0, .Y = -1, .Z = 0}, {.X = -1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = -1}),
              calculateAO({.X = 0, .Y = -1, .Z = 0}, {.X = 1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = -1}),
              calculateAO({.X = 0, .Y = -1, .Z = 0}, {.X = 1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = 1}),
              calculateAO({.X = 0, .Y = -1, .Z = 0}, {.X = -1, .Y = 0, .Z = 0}, {.X = 0, .Y = 0, .Z = 1}),
          };

        const auto worldZ = static_cast<float>(blocks.Depth() * chunkCoord.Z + z);
        const auto worldY = static_cast<float>(blocks.Height() * chunkCoord.Y + y);
        const auto worldX = static_cast<float>(blocks.Width() * chunkCoord.X + x);

        const std::uint32_t baseVertex = mesh.Vertices.size();
        buildVertices_(mesh, faces, ambientOcclusion, blocks[z, y, x], worldZ, worldY, worldX);
        buildIndices_(mesh, baseVertex, faces.NumEnabled());
      }
    }
  }
  assert(mesh.Vertices.size() <= std::numeric_limits<uint32_t>::max());
  return mesh;
}

void ChunkMesher::buildVertices_(ChunkMesh& mesh,
                                 const BlockFaces& faces,
                                 const FaceAmbientOcclusion& ambientOcclusion,
                                 const std::uint32_t blockType,
                                 float z,
                                 float y,
                                 float x) {
  constexpr float blockWidth = 1.0f;
  z *= blockWidth;
  y *= blockWidth;
  x *= blockWidth;

  constexpr size_t verticesPerFace = 4;
  const float x0 = x;
  const float x1 = x + blockWidth;
  const float y0 = y;
  const float y1 = y + blockWidth;
  const float z0 = z;
  const float z1 = z + blockWidth;

  std::vector<gfx::Vertex>& vertices = mesh.Vertices;
  vertices.reserve(vertices.size() + verticesPerFace * faces.NumEnabled());

  const auto& textures = blockRegistry_.GetBlockDef(static_cast<BlockType>(blockType)).FaceTextures;
  const auto color = [](const float ao) { return glm::vec3{ao, ao, ao}; };

  if (faces.Front) {
    const std::uint32_t faceIndex = static_cast<std::uint32_t>(textures[Front]);
    const auto& ao = ambientOcclusion[Front];
    // +Z (Front)
    vertices.insert(
        vertices.end(),
        {
            {.Position = {x0, y0, z1}, .Color = color(ao[0]), .TexCoord = {0, 0}, .TextureIndex = faceIndex},
            {.Position = {x1, y0, z1}, .Color = color(ao[1]), .TexCoord = {1, 0}, .TextureIndex = faceIndex},
            {.Position = {x1, y1, z1}, .Color = color(ao[2]), .TexCoord = {1, 1}, .TextureIndex = faceIndex},
            {.Position = {x0, y1, z1}, .Color = color(ao[3]), .TexCoord = {0, 1}, .TextureIndex = faceIndex},
    });
  }

  if (faces.Back) {
    const std::uint32_t faceIndex = static_cast<std::uint32_t>(textures[Back]);
    const auto& ao = ambientOcclusion[Back];
    // -Z (Back)
    vertices.insert(
        vertices.end(),
        {
            {.Position = {x1, y0, z0}, .Color = color(ao[0]), .TexCoord = {0, 0}, .TextureIndex = faceIndex},
            {.Position = {x0, y0, z0}, .Color = color(ao[1]), .TexCoord = {1, 0}, .TextureIndex = faceIndex},
            {.Position = {x0, y1, z0}, .Color = color(ao[2]), .TexCoord = {1, 1}, .TextureIndex = faceIndex},
            {.Position = {x1, y1, z0}, .Color = color(ao[3]), .TexCoord = {0, 1}, .TextureIndex = faceIndex},
    });
  }

  if (faces.Right) {
    const std::uint32_t faceIndex = static_cast<std::uint32_t>(textures[Right]);
    const auto& ao = ambientOcclusion[Right];
    // +X (Right)
    vertices.insert(
        vertices.end(),
        {
            {.Position = {x1, y0, z1}, .Color = color(ao[0]), .TexCoord = {0, 0}, .TextureIndex = faceIndex},
            {.Position = {x1, y0, z0}, .Color = color(ao[1]), .TexCoord = {1, 0}, .TextureIndex = faceIndex},
            {.Position = {x1, y1, z0}, .Color = color(ao[2]), .TexCoord = {1, 1}, .TextureIndex = faceIndex},
            {.Position = {x1, y1, z1}, .Color = color(ao[3]), .TexCoord = {0, 1}, .TextureIndex = faceIndex},
    });
  }

  if (faces.Left) {
    const std::uint32_t faceIndex = static_cast<std::uint32_t>(textures[Left]);
    const auto& ao = ambientOcclusion[Left];
    // -X (Left)
    vertices.insert(
        vertices.end(),
        {
            {.Position = {x0, y0, z0}, .Color = color(ao[0]), .TexCoord = {0, 0}, .TextureIndex = faceIndex},
            {.Position = {x0, y0, z1}, .Color = color(ao[1]), .TexCoord = {1, 0}, .TextureIndex = faceIndex},
            {.Position = {x0, y1, z1}, .Color = color(ao[2]), .TexCoord = {1, 1}, .TextureIndex = faceIndex},
            {.Position = {x0, y1, z0}, .Color = color(ao[3]), .TexCoord = {0, 1}, .TextureIndex = faceIndex},
    });
  }

  if (faces.Top) {
    const std::uint32_t faceIndex = static_cast<std::uint32_t>(textures[Top]);
    const auto& ao = ambientOcclusion[Top];
    // +Y (Top)
    vertices.insert(
        vertices.end(),
        {
            {.Position = {x0, y1, z1}, .Color = color(ao[0]), .TexCoord = {0, 0}, .TextureIndex = faceIndex},
            {.Position = {x1, y1, z1}, .Color = color(ao[1]), .TexCoord = {1, 0}, .TextureIndex = faceIndex},
            {.Position = {x1, y1, z0}, .Color = color(ao[2]), .TexCoord = {1, 1}, .TextureIndex = faceIndex},
            {.Position = {x0, y1, z0}, .Color = color(ao[3]), .TexCoord = {0, 1}, .TextureIndex = faceIndex},
    });
  }

  if (faces.Bottom) {
    const std::uint32_t faceIndex = static_cast<std::uint32_t>(textures[Bottom]);
    const auto& ao = ambientOcclusion[Bottom];
    // -Y (Bottom)
    vertices.insert(
        vertices.end(),
        {
            {.Position = {x0, y0, z0}, .Color = color(ao[0]), .TexCoord = {0, 0}, .TextureIndex = faceIndex},
            {.Position = {x1, y0, z0}, .Color = color(ao[1]), .TexCoord = {1, 0}, .TextureIndex = faceIndex},
            {.Position = {x1, y0, z1}, .Color = color(ao[2]), .TexCoord = {1, 1}, .TextureIndex = faceIndex},
            {.Position = {x0, y0, z1}, .Color = color(ao[3]), .TexCoord = {0, 1}, .TextureIndex = faceIndex},
    });
  }
}

void ChunkMesher::buildIndices_(ChunkMesh& mesh, const std::uint32_t baseVertex, const std::uint32_t numFaces) {
  constexpr std::size_t indicesPerFace = 6;
  constexpr std::uint32_t verticesPerFace = 4;
  static constexpr std::array<std::uint32_t, indicesPerFace> faceIndices{
      {0, 1, 2, 2, 3, 0}
  };

  std::vector<std::uint32_t>& indices = mesh.Indices;
  indices.reserve(indices.size() + indicesPerFace * numFaces);

  for (std::size_t i = 0; i < numFaces; i++) {
    for (const std::uint32_t idx : faceIndices) {
      indices.push_back(idx + baseVertex + i * verticesPerFace);
    };
  }
}
