#include "MapMesh.hpp"

#include "Grid3D.hpp"
#include "Map.hpp"

#include "Engine/Graphics/Vulkan/Renderer.hpp"

#include <cstddef>
#include <map>

const engine::graphics::MeshHandle& MapMesh::Mesh() const {
  return mesh_;
}

void MapMesh::Upload(engine::graphics::vulkan::Renderer& renderer) {
  mesh_ = renderer.CreateMesh({.Vertices = vertices_, .Indices = indices_});
}
void MapMesh::Build(const Map& map) {
  vertices_.clear();
  indices_.clear();

  for (size_t z = 0; z < map.Depth(); z++)
    for (size_t y = 0; y < map.Height(); y++)
      for (size_t x = 0; x < map.Width(); x++)
        buildChunk_(map, z, y, x);
}

void MapMesh::buildChunk_(const Map& map,
                          const std::uint32_t mapZ,
                          const std::uint32_t mapY,
                          const std::uint32_t mapX) {
  const auto& chunk = map.GetChunk(mapZ, mapY, mapX).Blocks;

  for (std::size_t z = 0; z < chunk.Depth(); z++) {
    for (std::size_t y = 0; y < chunk.Height(); y++) {
      for (std::size_t x = 0; x < chunk.Width(); x++) {

        if (chunk[z, y, x] == 0)
          continue;

        auto getBlock = [&](const int deltaZ, const int deltaY, const int deltaX) -> std::uint32_t {
          const int targetZ = static_cast<int>(z) + deltaZ;
          const int targetY = static_cast<int>(y) + deltaY;
          const int targetX = static_cast<int>(x) + deltaX;

          if (targetZ >= 0 && targetZ < chunk.Depth() && targetY >= 0 && targetY < chunk.Height() && targetX >= 0 &&
              targetX < chunk.Width())
            return chunk[targetZ, targetY, targetX];
          // Z Axis
          if (deltaZ == -1 && z == 0 && mapZ > 0) // Backwards (-z)
            return map.GetChunk(mapZ - 1, mapY, mapX).Blocks[chunk.Depth() - 1, y, x];
          if (deltaZ == +1 && z == chunk.Depth() - 1 && mapZ < map.Depth() - 1) // Forwards (+z);
            return map.GetChunk(mapZ + 1, mapY, mapX).Blocks[0, y, x];

          // Y Axis
          if (deltaY == -1 && y == 0 && mapY > 0) // Downwards (-y)
            return map.GetChunk(mapZ, mapY - 1, mapX).Blocks[z, chunk.Height() - 1, x];
          if (deltaY == 1 && y == chunk.Height() - 1 && mapY < map.Height() - 1) // Upwards (+y)
            return map.GetChunk(mapZ, mapY + 1, mapX).Blocks[z, 0, x];

          // X Axis
          if (deltaX == -1 && x == 0 && mapX > 0) // Left (-x)
            return map.GetChunk(mapZ, mapY, mapX - 1).Blocks[z, y, chunk.Width() - 1];
          if (deltaX == +1 && x == chunk.Width() - 1 && mapX < map.Width() - 1) // Right (+x)
            return map.GetChunk(mapZ, mapY, mapX + 1).Blocks[z, y, 0];

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

        const auto worldZ = static_cast<float>(chunk.Depth() * mapZ + z);
        const auto worldY = static_cast<float>(chunk.Height() * mapY + y);
        const auto worldX = static_cast<float>(chunk.Width() * mapX + x);

        const std::uint32_t nextIndex = vertices_.size();
        buildVertices_(faces, chunk[z, y, x], worldZ, worldY, worldX);
        buildIndices_(faces.NumEnabled(), nextIndex);
      }
    }
  }

  assert(vertices_.size() <= std::numeric_limits<uint32_t>::max());
}

void MapMesh::buildVertices_(const BlockFaces& faces, std::uint32_t blockType, float z, float y, float x) {
  constexpr float blockWidth = 1.0f;
  z *= blockWidth;
  y *= blockWidth;
  x *= blockWidth;

  constexpr size_t verticesPerFace = 4;

  // BlockType 0 represents air (no texture).
  // Texture array layers start at 0, so non-air block types are shifted down by 1.
  blockType -= 1;

  vertices_.reserve(vertices_.size() + verticesPerFace * faces.NumEnabled());

  if (faces.Front) {
    // +Z (Front)
    vertices_.insert(vertices_.end(),
                     {
                         {.Position = {-0.5f + x, -0.5f + y, 0.5f + z}, .TexCoord = {0, 0}, .TextureIndex = blockType},
                         {.Position = {0.5f + x, -0.5f + y, 0.5f + z},  .TexCoord = {1, 0}, .TextureIndex = blockType},
                         {.Position = {0.5f + x, 0.5f + y, 0.5f + z},   .TexCoord = {1, 1}, .TextureIndex = blockType},
                         {.Position = {-0.5f + x, 0.5f + y, 0.5f + z},  .TexCoord = {0, 1}, .TextureIndex = blockType},
    });
  }

  if (faces.Back) {
    // -Z (Back)
    vertices_.insert(vertices_.end(),
                     {
                         {.Position = {0.5f + x, -0.5f + y, -0.5f + z},  .TexCoord = {0, 0}, .TextureIndex = blockType},
                         {.Position = {-0.5f + x, -0.5f + y, -0.5f + z}, .TexCoord = {1, 0}, .TextureIndex = blockType},
                         {.Position = {-0.5f + x, 0.5f + y, -0.5f + z},  .TexCoord = {1, 1}, .TextureIndex = blockType},
                         {.Position = {0.5f + x, 0.5f + y, -0.5f + z},   .TexCoord = {0, 1}, .TextureIndex = blockType},
    });
  }

  if (faces.Right) {
    // +X (Right)
    vertices_.insert(vertices_.end(),
                     {
                         {.Position = {0.5f + x, -0.5f + y, 0.5f + z},  .TexCoord = {0, 0}, .TextureIndex = blockType},
                         {.Position = {0.5f + x, -0.5f + y, -0.5f + z}, .TexCoord = {1, 0}, .TextureIndex = blockType},
                         {.Position = {0.5f + x, 0.5f + y, -0.5f + z},  .TexCoord = {1, 1}, .TextureIndex = blockType},
                         {.Position = {0.5f + x, 0.5f + y, 0.5f + z},   .TexCoord = {0, 1}, .TextureIndex = blockType},
    });
  }

  if (faces.Left) {
    // -X (Left)
    vertices_.insert(vertices_.end(),
                     {
                         {.Position = {-0.5f + x, -0.5f + y, -0.5f + z}, .TexCoord = {0, 0}, .TextureIndex = blockType},
                         {.Position = {-0.5f + x, -0.5f + y, 0.5f + z},  .TexCoord = {1, 0}, .TextureIndex = blockType},
                         {.Position = {-0.5f + x, 0.5f + y, 0.5f + z},   .TexCoord = {1, 1}, .TextureIndex = blockType},
                         {.Position = {-0.5f + x, 0.5f + y, -0.5f + z},  .TexCoord = {0, 1}, .TextureIndex = blockType},
    });
  }

  if (faces.Top) {
    // +Y (Top)
    vertices_.insert(vertices_.end(),
                     {
                         {.Position = {-0.5f + x, 0.5f + y, 0.5f + z},  .TexCoord = {0, 0}, .TextureIndex = blockType},
                         {.Position = {0.5f + x, 0.5f + y, 0.5f + z},   .TexCoord = {1, 0}, .TextureIndex = blockType},
                         {.Position = {0.5f + x, 0.5f + y, -0.5f + z},  .TexCoord = {1, 1}, .TextureIndex = blockType},
                         {.Position = {-0.5f + x, 0.5f + y, -0.5f + z}, .TexCoord = {0, 1}, .TextureIndex = blockType},
    });
  }

  if (faces.Bottom) {
    // -Y (Bottom)
    vertices_.insert(vertices_.end(),
                     {
                         {.Position = {-0.5f + x, -0.5f + y, -0.5f + z}, .TexCoord = {0, 0}, .TextureIndex = blockType},
                         {.Position = {0.5f + x, -0.5f + y, -0.5f + z},  .TexCoord = {1, 0}, .TextureIndex = blockType},
                         {.Position = {0.5f + x, -0.5f + y, 0.5f + z},   .TexCoord = {1, 1}, .TextureIndex = blockType},
                         {.Position = {-0.5f + x, -0.5f + y, 0.5f + z},  .TexCoord = {0, 1}, .TextureIndex = blockType},
    });
  }
}

void MapMesh::buildIndices_(const std::uint32_t numFaces, std::uint32_t baseVertex) {
  constexpr std::uint32_t indicesPerFace = 6;
  static constexpr std::array<std::uint32_t, indicesPerFace> faceIndices{
      {0, 1, 2, 2, 3, 0}
  };
  constexpr std::uint32_t verticesPerFace = 4;

  indices_.reserve(indices_.size() + static_cast<std::size_t>(indicesPerFace) * numFaces);

  for (std::size_t i = 0; i < numFaces; i++) {
    for (const std::uint32_t idx : faceIndices) {
      indices_.push_back(idx + baseVertex);
    };
    baseVertex += verticesPerFace;
  }
}