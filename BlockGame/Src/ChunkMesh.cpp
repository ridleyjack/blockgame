#include "ChunkMesh.h"

#include "Grid3D.hpp"

#include "Engine/Graphics/Vulkan/Renderer.hpp"

#include <cstddef>

ChunkMesh::ChunkMesh() {
  build_();
}

const engine::graphics::MeshHandle& ChunkMesh::Mesh() const {
  return mesh_;
}

void ChunkMesh::Upload(engine::graphics::vulkan::Renderer& renderer) {
  mesh_ = renderer.CreateMesh({.Vertices = vertices_, .Indices = indices_});
}

void ChunkMesh::build_() {
  Grid3D<uint8_t> map{16, 16, 16, 1};

  for (std::size_t z = 0; z < map.Depth(); z++) {
    for (std::size_t y = 0; y < map.Height(); y++) {
      for (std::size_t x = 0; x < map.Width(); x++) {
        if (x == 0) {
          map[z, y, x] = 2;
        }
        if (x == map.Width() - 1) {
          map[z, y, x] = 3;
        }
        if (z == 0) {
          map[z, y, x] = 4;
        }
      }
    }
  }

  map[4, 4, 4] = 0;
  map[0, 0, 0] = 0;

  for (std::size_t z = 0; z < map.Depth(); z++) {
    for (std::size_t y = 0; y < map.Height(); y++) {
      for (std::size_t x = 0; x < map.Width(); x++) {

        if (map[z, y, x] == 0)
          continue;

        BlockFaces faces{};
        if (z == 0 || map[z - 1, y, x] == 0)
          faces.Back = true;
        if (z == map.Depth() - 1 || map[z + 1, y, x] == 0)
          faces.Front = true;
        if (y == 0 || map[z, y - 1, x] == 0)
          faces.Bottom = true;
        if (y == map.Height() - 1 || map[z, y + 1, x] == 0)
          faces.Top = true;
        if (x == 0 || map[z, y, x - 1] == 0)
          faces.Left = true;
        if (x == map.Width() - 1 || map[z, y, x + 1] == 0)
          faces.Right = true;

        const std::uint32_t nextIndex = vertices_.size();
        buildVertices_(faces, map[z, y, x], static_cast<float>(z), static_cast<float>(y), static_cast<float>(x));
        buildIndices_(faces.NumEnabled(), nextIndex);
      }
    }
  }

  assert(vertices_.size() <= std::numeric_limits<uint32_t>::max());
}

void ChunkMesh::buildVertices_(const BlockFaces& faces, std::uint32_t blockType, float z, float y, float x) {
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

void ChunkMesh::buildIndices_(const std::uint32_t numFaces, std::uint32_t baseVertex) {
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