#include "ChunkMeshes.hpp"

#include "Grid3D.hpp"
#include "Map.hpp"

#include "Engine/Graphics/Vulkan/Renderer.hpp"

#include <cstddef>
#include <map>

ChunkMeshes::ChunkMeshes(Map& map) : map_(map), meshes_(map.Depth(), map.Height(), map.Width(), {}) {}

const ChunkMesh& ChunkMeshes::Mesh(const math::Vec3Int& mapCoord) const {
  assert(mapCoord.Z < meshes_.Depth() && mapCoord.Y < meshes_.Height() && mapCoord.X < meshes_.Width());
  return meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X];
}

void ChunkMeshes::BuildAll(vlk::Renderer& renderer) {
  for (std::int32_t z = 0; z < map_.Depth(); z++)
    for (std::int32_t y = 0; y < map_.Height(); y++)
      for (std::int32_t x = 0; x < map_.Width(); x++) {
        meshes_[z, y, x].Vertices.clear();
        meshes_[z, y, x].Indices.clear();
        buildChunk_({z, y, x});
        upload_(renderer, meshes_[z, y, x]);
      }
}

void ChunkMeshes::BuildChunk(vlk::Renderer& renderer, const math::Vec3Int& mapCoord) {
  assert(mapCoord.Z < meshes_.Depth() && mapCoord.Y < meshes_.Height() && mapCoord.X < meshes_.Width());
  ChunkMesh& mesh = meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X];
  mesh.Vertices.clear();
  mesh.Indices.clear();

  buildChunk_(mapCoord);
  upload_(renderer, mesh);
}

void ChunkMeshes::buildChunk_(const math::Vec3Int& mapCoord) {
  const auto& chunk = map_.GetChunk(mapCoord.Z, mapCoord.Y, mapCoord.X).Blocks;

  for (std::int32_t z = 0; z < chunk.Depth(); z++) {
    for (std::int32_t y = 0; y < chunk.Height(); y++) {
      for (std::int32_t x = 0; x < chunk.Width(); x++) {

        if (chunk[z, y, x] == 0)
          continue;

        auto getBlock = [&](const int deltaZ, const int deltaY, const int deltaX) -> std::uint32_t {
          const int targetZ = z + deltaZ;
          const int targetY = y + deltaY;
          const int targetX = x + deltaX;

          if (targetZ >= 0 && targetZ < chunk.Depth() && targetY >= 0 && targetY < chunk.Height() && targetX >= 0 &&
              targetX < chunk.Width())
            return chunk[targetZ, targetY, targetX];
          // Z Axis
          if (deltaZ == -1 && z == 0 && mapCoord.Z > 0) // Backwards (-z)
            return map_.GetChunk(mapCoord.Z - 1, mapCoord.Y, mapCoord.X).Blocks[chunk.Depth() - 1, y, x];
          if (deltaZ == +1 && z == chunk.Depth() - 1 && mapCoord.Z < map_.Depth() - 1) // Forwards (+z);
            return map_.GetChunk(mapCoord.Z + 1, mapCoord.Y, mapCoord.X).Blocks[0, y, x];

          // Y Axis
          if (deltaY == -1 && y == 0 && mapCoord.Y > 0) // Downwards (-y)
            return map_.GetChunk(mapCoord.Z, mapCoord.Y - 1, mapCoord.X).Blocks[z, chunk.Height() - 1, x];
          if (deltaY == 1 && y == chunk.Height() - 1 && mapCoord.Y < map_.Height() - 1) // Upwards (+y)
            return map_.GetChunk(mapCoord.Z, mapCoord.Y + 1, mapCoord.X).Blocks[z, 0, x];

          // X Axis
          if (deltaX == -1 && x == 0 && mapCoord.X > 0) // Left (-x)
            return map_.GetChunk(mapCoord.Z, mapCoord.Y, mapCoord.X - 1).Blocks[z, y, chunk.Width() - 1];
          if (deltaX == +1 && x == chunk.Width() - 1 && mapCoord.X < map_.Width() - 1) // Right (+x)
            return map_.GetChunk(mapCoord.Z, mapCoord.Y, mapCoord.X + 1).Blocks[z, y, 0];

          return 0; // Air if out of bounds.
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

        const auto worldZ = static_cast<float>(chunk.Depth() * mapCoord.Z + z);
        const auto worldY = static_cast<float>(chunk.Height() * mapCoord.Y + y);
        const auto worldX = static_cast<float>(chunk.Width() * mapCoord.X + x);

        // TODO: This can be removed unless I add vertices to existing meshes in the future.
        ChunkMesh& mesh = meshes_[mapCoord.Z, mapCoord.Y, mapCoord.X];
        const std::uint32_t nextIndex = mesh.Vertices.size();

        buildVertices_(mesh, faces, chunk[z, y, x], worldZ, worldY, worldX);
        buildIndices_(mesh, faces.NumEnabled(), nextIndex);

        assert(mesh.Vertices.size() <= std::numeric_limits<uint32_t>::max());
      }
    }
  }
}

void ChunkMeshes::buildVertices_(ChunkMesh& mesh,
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

  // BlockType 0 represents air (no texture).
  // Texture array layers start at 0, so non-air block types are shifted down by 1.
  blockType -= 1;

  auto& vertices = mesh.Vertices;
  vertices.reserve(vertices.size() + verticesPerFace * faces.NumEnabled());

  if (faces.Front) {
    // +Z (Front)
    vertices.insert(vertices.end(),
                    {
                        {.Position = {-0.5f + x, -0.5f + y, 0.5f + z}, .TexCoord = {0, 0}, .TextureIndex = blockType},
                        {.Position = {0.5f + x, -0.5f + y, 0.5f + z},  .TexCoord = {1, 0}, .TextureIndex = blockType},
                        {.Position = {0.5f + x, 0.5f + y, 0.5f + z},   .TexCoord = {1, 1}, .TextureIndex = blockType},
                        {.Position = {-0.5f + x, 0.5f + y, 0.5f + z},  .TexCoord = {0, 1}, .TextureIndex = blockType},
    });
  }

  if (faces.Back) {
    // -Z (Back)
    vertices.insert(vertices.end(),
                    {
                        {.Position = {0.5f + x, -0.5f + y, -0.5f + z},  .TexCoord = {0, 0}, .TextureIndex = blockType},
                        {.Position = {-0.5f + x, -0.5f + y, -0.5f + z}, .TexCoord = {1, 0}, .TextureIndex = blockType},
                        {.Position = {-0.5f + x, 0.5f + y, -0.5f + z},  .TexCoord = {1, 1}, .TextureIndex = blockType},
                        {.Position = {0.5f + x, 0.5f + y, -0.5f + z},   .TexCoord = {0, 1}, .TextureIndex = blockType},
    });
  }

  if (faces.Right) {
    // +X (Right)
    vertices.insert(vertices.end(),
                    {
                        {.Position = {0.5f + x, -0.5f + y, 0.5f + z},  .TexCoord = {0, 0}, .TextureIndex = blockType},
                        {.Position = {0.5f + x, -0.5f + y, -0.5f + z}, .TexCoord = {1, 0}, .TextureIndex = blockType},
                        {.Position = {0.5f + x, 0.5f + y, -0.5f + z},  .TexCoord = {1, 1}, .TextureIndex = blockType},
                        {.Position = {0.5f + x, 0.5f + y, 0.5f + z},   .TexCoord = {0, 1}, .TextureIndex = blockType},
    });
  }

  if (faces.Left) {
    // -X (Left)
    vertices.insert(vertices.end(),
                    {
                        {.Position = {-0.5f + x, -0.5f + y, -0.5f + z}, .TexCoord = {0, 0}, .TextureIndex = blockType},
                        {.Position = {-0.5f + x, -0.5f + y, 0.5f + z},  .TexCoord = {1, 0}, .TextureIndex = blockType},
                        {.Position = {-0.5f + x, 0.5f + y, 0.5f + z},   .TexCoord = {1, 1}, .TextureIndex = blockType},
                        {.Position = {-0.5f + x, 0.5f + y, -0.5f + z},  .TexCoord = {0, 1}, .TextureIndex = blockType},
    });
  }

  if (faces.Top) {
    // +Y (Top)
    vertices.insert(vertices.end(),
                    {
                        {.Position = {-0.5f + x, 0.5f + y, 0.5f + z},  .TexCoord = {0, 0}, .TextureIndex = blockType},
                        {.Position = {0.5f + x, 0.5f + y, 0.5f + z},   .TexCoord = {1, 0}, .TextureIndex = blockType},
                        {.Position = {0.5f + x, 0.5f + y, -0.5f + z},  .TexCoord = {1, 1}, .TextureIndex = blockType},
                        {.Position = {-0.5f + x, 0.5f + y, -0.5f + z}, .TexCoord = {0, 1}, .TextureIndex = blockType},
    });
  }

  if (faces.Bottom) {
    // -Y (Bottom)
    vertices.insert(vertices.end(),
                    {
                        {.Position = {-0.5f + x, -0.5f + y, -0.5f + z}, .TexCoord = {0, 0}, .TextureIndex = blockType},
                        {.Position = {0.5f + x, -0.5f + y, -0.5f + z},  .TexCoord = {1, 0}, .TextureIndex = blockType},
                        {.Position = {0.5f + x, -0.5f + y, 0.5f + z},   .TexCoord = {1, 1}, .TextureIndex = blockType},
                        {.Position = {-0.5f + x, -0.5f + y, 0.5f + z},  .TexCoord = {0, 1}, .TextureIndex = blockType},
    });
  }
}

void ChunkMeshes::buildIndices_(ChunkMesh& mesh, const std::uint32_t numFaces, std::uint32_t baseVertex) {
  constexpr std::uint32_t indicesPerFace = 6;
  static constexpr std::array<std::uint32_t, indicesPerFace> faceIndices{
      {0, 1, 2, 2, 3, 0}
  };
  constexpr std::uint32_t verticesPerFace = 4;

  mesh.Indices.reserve(mesh.Indices.size() + static_cast<std::size_t>(indicesPerFace) * numFaces);

  for (std::size_t i = 0; i < numFaces; i++) {
    for (const std::uint32_t idx : faceIndices) {
      mesh.Indices.push_back(idx + baseVertex);
    };
    baseVertex += verticesPerFace;
  }
}

void ChunkMeshes::upload_(vlk::Renderer& renderer, ChunkMesh& mesh) {
  if (!mesh.HasVertices())
    return;

  auto handle = renderer.CreateMesh({.Vertices = mesh.Vertices, .Indices = mesh.Indices});
  mesh.Mesh = handle;
}