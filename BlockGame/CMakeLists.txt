add_executable(Blockgame
        Src/main.cpp
        Src/GameLayer.cpp
        Src/GameLayer.hpp
        Src/Camera.cpp
        Src/Camera.hpp
        Src/Containers/Grid3D.hpp
        Src/Containers/ThreadSafeQueue.hpp
        Src/World/ChunkMesher.cpp
        Src/World/ChunkMesher.hpp
        Src/BlockRegistry.cpp
        Src/BlockRegistry.hpp
        Src/TextureRegistry.cpp
        Src/TextureRegistry.hpp
        Src/World/ChunkStreamer.cpp
        Src/World/ChunkStreamer.hpp
        Src/World/WorldGenerator.cpp
        Src/World/WorldGenerator.hpp
        Src/World/World.cpp
        Src/World/World.hpp
        Src/BlockTypes.hpp
        Src/World/Chunk.hpp
        Src/World/WorldStore.cpp
        Src/World/WorldStore.hpp
)

target_include_directories(Blockgame
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_link_libraries(Blockgame
        PRIVATE
        Engine
        Engine_Graphics
        glfw
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Textures
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Copy shaders to the build directory.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
