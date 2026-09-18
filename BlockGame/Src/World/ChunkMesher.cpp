#include "ChunkMesher.hpp"

#include "BlockRegistry.hpp"
#include "WorldStore.hpp"
#include "Containers/Grid3D.hpp"
#include "Containers/WorkerPool.hpp"

#include "Engine/Graphics/Vulkan/Renderer.hpp"

#include <cstddef>
#include <utility>

ChunkMesher::ChunkMesher(vlk::Renderer& renderer,
                         WorkerPool& workerPool,
                         WorldStore& worldStore,
                         BlockRegistry& blockRegistry)
    : renderer_(renderer),
      workerPool_(workerPool),
      worldStore_(worldStore),
      meshes_(WorldStore::WorldDepth, WorldStore::WorldHeight, WorldStore::WorldWidth, {}),
      blockRegistry_(blockRegistry) {}

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

void ChunkMesher::enqueueBuild_(const math::Vec3Int mapCoord) {
  ChunkMeshSlot& meshSlot = meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X];

  meshSlot.Wanted = true;
  meshSlot.Generation++;
  meshSlot.Status = ChunkMeshStatus::Building;

  const std::uint64_t generation = meshSlot.Generation;

  auto buildJob = [this, mapCoord, generation] {
    ChunkBuildResult result{
        .Coord = mapCoord,
        .Generation = generation,
    };

    const auto worldView = worldStore_.AcquireReadView();

    if (!buildDependenciesReady_(worldView, mapCoord)) {
      result.Status = ChunkMeshStatus::MissingDependencies;
    } else {
      result.Mesh = buildChunk_(worldView, mapCoord);
      result.Status = ChunkMeshStatus::Building;
    }

    resultQueue_.Push(std::move(result));
  };

  if (!workerPool_.Enqueue(buildJob)) {
    meshSlot.Status =
        meshSlot.VisibleHandle || meshSlot.PendingHandle ? ChunkMeshStatus::Uploaded : ChunkMeshStatus::Unloaded;
  }
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

        const auto block = blocks[z, y, x];
        if (block == std::to_underlying(BlockType::Air))
          continue;

        math::Vec3Int blockCoord{x, y, z};

        auto& blockDef = blockRegistry_.GetBlockDef(static_cast<BlockType>(block));
        switch (blockDef.Shape) {
        case BlockShape::Cube:
          buildSolidBlock_(worldView, mesh, chunkCoord, blockCoord);
          break;
        case BlockShape::CrossBillboard:
          buildCrossBillboardBlock_(mesh, chunkCoord, blockCoord, static_cast<BlockType>(block));
          break;
        }
      }
    }
  }
  assert(mesh.Vertices.size() <= std::numeric_limits<uint32_t>::max());
  return mesh;
}

void ChunkMesher::buildSolidBlock_(const WorldStore::ReadView& worldView,
                                   ChunkMesh& mesh,
                                   const math::Vec3Int chunkCoord,
                                   const math::Vec3Int blockCoord) {

  const auto chunk = worldView.GetChunk(chunkCoord);
  auto& blocks = chunk->Blocks;

  math::Vec3Int worldCoord{.X = static_cast<std::int32_t>(chunkCoord.X * blocks.Width()) + blockCoord.X,
                           .Y = static_cast<std::int32_t>(chunkCoord.Y * blocks.Height()) + blockCoord.Y,
                           .Z = static_cast<std::int32_t>(chunkCoord.Z * blocks.Depth()) + blockCoord.Z};

  auto getBlock = [&](const int deltaZ, const int deltaY, const int deltaX) -> std::uint32_t {
    math::Vec3Int target{
        .X = blockCoord.X + deltaX,
        .Y = blockCoord.Y + deltaY,
        .Z = blockCoord.Z + deltaZ,
    };

    if (target.Z >= 0 && target.Z < blocks.Depth() && target.Y >= 0 && target.Y < blocks.Height() && target.X >= 0 &&
        target.X < blocks.Width())
      return blocks[target.Z, target.Y, target.X];

    // Shift target block coordinates from chunk local to world coordinates.
    target.X += chunkCoord.X * static_cast<std::int32_t>(chunk->ChunkWidth);
    target.Y += chunkCoord.Y * static_cast<std::int32_t>(chunk->ChunkHeight);
    target.Z += chunkCoord.Z * static_cast<std::int32_t>(chunk->ChunkDepth);

    if (target.Z < 0 || target.Z >= WorldStore::WorldDepth * Chunk::ChunkDepth || target.Y < 0 ||
        target.Y >= WorldStore::WorldHeight * Chunk::ChunkHeight || target.X < 0 ||
        target.X >= WorldStore::WorldWidth * Chunk::ChunkWidth)
      return 0;

    const Chunk* neighbour = worldView.GetChunk({static_cast<std::int32_t>(target.X / Chunk::ChunkWidth),
                                                 static_cast<std::int32_t>(target.Y / Chunk::ChunkHeight),
                                                 static_cast<std::int32_t>(target.Z / Chunk::ChunkDepth)});
    assert(neighbour != nullptr);
    return neighbour->Blocks[target.Z % Chunk::ChunkDepth, target.Y % Chunk::ChunkHeight, target.X % Chunk::ChunkWidth];
  };

  auto isOpaque = [&](std::uint32_t blockType) -> bool {
    return blockRegistry_.GetBlockDef(static_cast<BlockType>(blockType)).Opaque;
  };

  BlockFaces faces{};
  if (!isOpaque(getBlock(-1, 0, 0)))
    faces.Back = true;
  if (!isOpaque(getBlock(+1, 0, 0)))
    faces.Front = true;

  if (!isOpaque(getBlock(0, -1, 0)))
    faces.Bottom = true;
  if (!isOpaque(getBlock(0, 1, 0)))
    faces.Top = true;

  if (!isOpaque(getBlock(0, 0, -1)))
    faces.Left = true;
  if (!isOpaque(getBlock(0, 0, 1)))
    faces.Right = true;

  if (faces.NumEnabled() == 0)
    return;

  auto calculateAO = [&](const math::Vec3Int normal, const math::Vec3Int sideA, const math::Vec3Int sideB) {
    const auto occludes = [&](const math::Vec3Int offset) { return isOpaque(getBlock(offset.Z, offset.Y, offset.X)); };

    const bool a = occludes({.X = normal.X + sideA.X, .Y = normal.Y + sideA.Y, .Z = normal.Z + sideA.Z});
    const bool b = occludes({.X = normal.X + sideB.X, .Y = normal.Y + sideB.Y, .Z = normal.Z + sideB.Z});
    const bool corner = occludes(
        {.X = normal.X + sideA.X + sideB.X, .Y = normal.Y + sideA.Y + sideB.Y, .Z = normal.Z + sideA.Z + sideB.Z});

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

  const std::uint32_t baseVertex = mesh.Vertices.size();
  buildSolidBlockVertices_(mesh,
                           faces,
                           ambientOcclusion,
                           blocks[blockCoord.Z, blockCoord.Y, blockCoord.X],
                           static_cast<float>(worldCoord.Z),
                           static_cast<float>(worldCoord.Y),
                           static_cast<float>(worldCoord.X));
  buildIndices_(mesh, baseVertex, faces.NumEnabled());
}

void ChunkMesher::buildCrossBillboardBlock_(ChunkMesh& mesh,
                                            math::Vec3Int chunkCoord,
                                            math::Vec3Int blockCoord,
                                            BlockType type) {
  const auto& def = blockRegistry_.GetBlockDef(type);
  const std::uint32_t tex = static_cast<std::uint32_t>(def.FaceTextures[Front]);

  const float x0 = static_cast<float>(chunkCoord.X * Chunk::ChunkWidth + blockCoord.X) + 0.1f;
  const float x1 = static_cast<float>(chunkCoord.X * Chunk::ChunkWidth + blockCoord.X) + 0.9f;
  const float y0 = static_cast<float>(chunkCoord.Y * Chunk::ChunkHeight + blockCoord.Y);
  const float y1 = static_cast<float>(chunkCoord.Y * Chunk::ChunkHeight + blockCoord.Y) + 0.9f;
  const float z0 = static_cast<float>(chunkCoord.Z * Chunk::ChunkDepth + blockCoord.Z) + 0.1f;
  const float z1 = static_cast<float>(chunkCoord.Z * Chunk::ChunkDepth + blockCoord.Z) + 0.9f;

  constexpr glm::vec3 color{1.0f, 1.0f, 1.0f};
  const std::uint32_t base = mesh.Vertices.size();

  mesh.Vertices.insert(mesh.Vertices.end(),
                       {
	                           // Diagonal plane 1
	                           {{x0, y0, z0}, color, {0, 0}, tex},
	                           {{x1, y0, z1}, color, {1, 0}, tex},
	                           {{x1, y1, z1}, color, {1, 1}, tex},
	                           {{x0, y1, z0}, color, {0, 1}, tex},
	                           // Diagonal plane 1 back face
	                           {{x0, y1, z0}, color, {0, 1}, tex},
	                           {{x1, y1, z1}, color, {1, 1}, tex},
	                           {{x1, y0, z1}, color, {1, 0}, tex},
	                           {{x0, y0, z0}, color, {0, 0}, tex},
	                           // Diagonal plane 2
	                           {{x1, y0, z0}, color, {0, 0}, tex},
	                           {{x0, y0, z1}, color, {1, 0}, tex},
	                           {{x0, y1, z1}, color, {1, 1}, tex},
	                           {{x1, y1, z0}, color, {0, 1}, tex},
	                           // Diagonal plane 2 back face
	                           {{x1, y1, z0}, color, {0, 1}, tex},
	                           {{x0, y1, z1}, color, {1, 1}, tex},
	                           {{x0, y0, z1}, color, {1, 0}, tex},
	                           {{x1, y0, z0}, color, {0, 0}, tex}
	  });

	  buildIndices_(mesh, base, 4);
}

void ChunkMesher::buildSolidBlockVertices_(ChunkMesh& mesh,
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
