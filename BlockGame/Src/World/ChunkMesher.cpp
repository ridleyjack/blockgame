#include "ChunkMesher.hpp"

#include "BlockRegistry.hpp"
#include "WorldStore.hpp"
#include "Containers/Grid3D.hpp"

#include "Engine/Graphics/Vulkan/Renderer.hpp"

#include <cstddef>
#include <print>
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

const ChunkMesh& ChunkMesher::Mesh(const math::Vec3Int& mapCoord) const {
  assert(mapCoord.Z < meshes_.Depth() && mapCoord.Y < meshes_.Height() && mapCoord.X < meshes_.Width());
  return meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X].Mesh;
}

void ChunkMesher::RequestLoad(const math::Vec3Int& mapCoord) {
  enqueueBuild_(mapCoord);
}

void ChunkMesher::RequestUnload(const math::Vec3Int& mapCoord) {
  auto& meshSlot = meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X];
  auto& mesh = meshSlot.Mesh;

  meshSlot.Wanted = false;
  if (meshSlot.Status == ChunkMeshStatus::Uploaded) {
    if (mesh.HasVertices())
      renderer_.DeleteMesh(mesh.Mesh);
    meshSlot.Status = ChunkMeshStatus::Unloaded;
    mesh = {};
  }
}

ChunkMeshStatus ChunkMesher::ChunkStatus(const math::Vec3Int& mapCoord) {
  return meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X].Status;
}

void ChunkMesher::Update() {
  while (auto opt = resultQueue_.TryPop()) {
    auto& result = *opt;

    auto& meshSlot = meshes_[result.Coord.Z, result.Coord.Y, result.Coord.X];
    if (meshSlot.Wanted) {
      if (result.Mesh.HasVertices()) {
        const auto handle = renderer_.CreateMesh({.Vertices = result.Mesh.Vertices, .Indices = result.Mesh.Indices});
        meshSlot.Mesh = std::move(result.Mesh);
        meshSlot.Mesh.Mesh = handle;
      } else {
        meshSlot.Mesh = {};
      }
      meshSlot.Status = ChunkMeshStatus::Uploaded;

    } else {
      meshSlot.Mesh = {};
      meshSlot.Status = ChunkMeshStatus::Unloaded;
    }
  }
}

void ChunkMesher::startWorkers_(std::uint32_t count) {
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
    ChunkMesh mesh = buildChunk_(job->Coord);
    resultQueue_.Push({job->Coord, std::move(mesh)});
  }
}

void ChunkMesher::enqueueBuild_(math::Vec3Int mapCoord) {
  ChunkMeshSlot& meshSlot = meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X];
  meshSlot.Wanted = true;
  if (meshSlot.Status == ChunkMeshStatus::Building || meshSlot.Status == ChunkMeshStatus::Uploaded)
    return;

  meshSlot.Status = ChunkMeshStatus::Building;
  buildQueue_.Push({mapCoord});
}

ChunkMesh ChunkMesher::buildChunk_(math::Vec3Int chunkCoord) {
  WorldStore::ReadLock readLock = worldStore_.AcquireReadLock();
  auto chunk = worldStore_.TryGetChunk(chunkCoord);
  if (chunk == nullptr)
    throw std::runtime_error("Generating mesh for un-created chunk");
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

          Chunk* neighbour = worldStore_.TryGetChunk({static_cast<std::int32_t>(worldCoord.X / Chunk::ChunkWidth),
                                                      static_cast<std::int32_t>(worldCoord.Y / Chunk::ChunkHeight),
                                                      static_cast<std::int32_t>(worldCoord.Z / Chunk::ChunkDepth)});
          if (neighbour == nullptr)
            throw std::runtime_error("Missing neighbour chunk when generating mesh");

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

        const auto worldZ = static_cast<float>(blocks.Depth() * chunkCoord.Z + z);
        const auto worldY = static_cast<float>(blocks.Height() * chunkCoord.Y + y);
        const auto worldX = static_cast<float>(blocks.Width() * chunkCoord.X + x);

        const std::uint32_t baseVertex = mesh.Vertices.size();
        buildVertices_(mesh, faces, blocks[z, y, x], worldZ, worldY, worldX);
        buildIndices_(mesh, baseVertex, faces.NumEnabled());
      }
    }
  }
  assert(mesh.Vertices.size() <= std::numeric_limits<uint32_t>::max());
  return mesh;
}

void ChunkMesher::buildVertices_(ChunkMesh& mesh,
                                 const BlockFaces& faces,
                                 std::uint32_t blockType,
                                 float z,
                                 float y,
                                 float x) {
  constexpr float blockWidth = 1.0f;
  z *= blockWidth;
  y *= blockWidth;
  x *= blockWidth;

  constexpr size_t verticesPerFace = 4;

  std::vector<gfx::Vertex>& vertices = mesh.Vertices;
  vertices.reserve(vertices.size() + verticesPerFace * faces.NumEnabled());

  const auto& textures = blockRegistry_.GetBlockDef(static_cast<BlockType>(blockType)).FaceTextures;
  if (faces.Front) {
    const std::uint32_t faceIndex = static_cast<std::uint32_t>(textures[Front]);
    // +Z (Front)
    vertices.insert(vertices.end(),
                    {
                        {.Position = {-0.5f + x, -0.5f + y, 0.5f + z}, .TexCoord = {0, 0}, .TextureIndex = faceIndex},
                        {.Position = {0.5f + x, -0.5f + y, 0.5f + z},  .TexCoord = {1, 0}, .TextureIndex = faceIndex},
                        {.Position = {0.5f + x, 0.5f + y, 0.5f + z},   .TexCoord = {1, 1}, .TextureIndex = faceIndex},
                        {.Position = {-0.5f + x, 0.5f + y, 0.5f + z},  .TexCoord = {0, 1}, .TextureIndex = faceIndex},
    });
  }

  if (faces.Back) {
    const std::uint32_t faceIndex = static_cast<std::uint32_t>(textures[Back]);
    // -Z (Back)
    vertices.insert(vertices.end(),
                    {
                        {.Position = {0.5f + x, -0.5f + y, -0.5f + z},  .TexCoord = {0, 0}, .TextureIndex = faceIndex},
                        {.Position = {-0.5f + x, -0.5f + y, -0.5f + z}, .TexCoord = {1, 0}, .TextureIndex = faceIndex},
                        {.Position = {-0.5f + x, 0.5f + y, -0.5f + z},  .TexCoord = {1, 1}, .TextureIndex = faceIndex},
                        {.Position = {0.5f + x, 0.5f + y, -0.5f + z},   .TexCoord = {0, 1}, .TextureIndex = faceIndex},
    });
  }

  if (faces.Right) {
    const std::uint32_t faceIndex = static_cast<std::uint32_t>(textures[Back]);
    // +X (Right)
    vertices.insert(vertices.end(),
                    {
                        {.Position = {0.5f + x, -0.5f + y, 0.5f + z},  .TexCoord = {0, 0}, .TextureIndex = faceIndex},
                        {.Position = {0.5f + x, -0.5f + y, -0.5f + z}, .TexCoord = {1, 0}, .TextureIndex = faceIndex},
                        {.Position = {0.5f + x, 0.5f + y, -0.5f + z},  .TexCoord = {1, 1}, .TextureIndex = faceIndex},
                        {.Position = {0.5f + x, 0.5f + y, 0.5f + z},   .TexCoord = {0, 1}, .TextureIndex = faceIndex},
    });
  }

  if (faces.Left) {
    const std::uint32_t faceIndex = static_cast<std::uint32_t>(textures[Left]);
    // -X (Left)
    vertices.insert(vertices.end(),
                    {
                        {.Position = {-0.5f + x, -0.5f + y, -0.5f + z}, .TexCoord = {0, 0}, .TextureIndex = faceIndex},
                        {.Position = {-0.5f + x, -0.5f + y, 0.5f + z},  .TexCoord = {1, 0}, .TextureIndex = faceIndex},
                        {.Position = {-0.5f + x, 0.5f + y, 0.5f + z},   .TexCoord = {1, 1}, .TextureIndex = faceIndex},
                        {.Position = {-0.5f + x, 0.5f + y, -0.5f + z},  .TexCoord = {0, 1}, .TextureIndex = faceIndex},
    });
  }

  if (faces.Top) {
    const std::uint32_t faceIndex = static_cast<std::uint32_t>(textures[Top]);
    // +Y (Top)
    vertices.insert(vertices.end(),
                    {
                        {.Position = {-0.5f + x, 0.5f + y, 0.5f + z},  .TexCoord = {0, 0}, .TextureIndex = faceIndex},
                        {.Position = {0.5f + x, 0.5f + y, 0.5f + z},   .TexCoord = {1, 0}, .TextureIndex = faceIndex},
                        {.Position = {0.5f + x, 0.5f + y, -0.5f + z},  .TexCoord = {1, 1}, .TextureIndex = faceIndex},
                        {.Position = {-0.5f + x, 0.5f + y, -0.5f + z}, .TexCoord = {0, 1}, .TextureIndex = faceIndex},
    });
  }

  if (faces.Bottom) {
    const std::uint32_t faceIndex = static_cast<std::uint32_t>(textures[Bottom]);
    // -Y (Bottom)
    vertices.insert(vertices.end(),
                    {
                        {.Position = {-0.5f + x, -0.5f + y, -0.5f + z}, .TexCoord = {0, 0}, .TextureIndex = faceIndex},
                        {.Position = {0.5f + x, -0.5f + y, -0.5f + z},  .TexCoord = {1, 0}, .TextureIndex = faceIndex},
                        {.Position = {0.5f + x, -0.5f + y, 0.5f + z},   .TexCoord = {1, 1}, .TextureIndex = faceIndex},
                        {.Position = {-0.5f + x, -0.5f + y, 0.5f + z},  .TexCoord = {0, 1}, .TextureIndex = faceIndex},
    });
  }
}

void ChunkMesher::buildIndices_(ChunkMesh& mesh, std::uint32_t baseVertex, const std::uint32_t numFaces) {
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
