add_executable(Blockgame
        Src/main.cpp
        Src/GameLayer.cpp
        Src/GameLayer.hpp
        Src/Camera.cpp
        Src/Camera.hpp
        Src/Grid3D.hpp
        Src/MapMesh.cpp
        Src/MapMesh.hpp
        Src/Map.cpp
        Src/Map.hpp
)

target_link_libraries(Blockgame
        PRIVATE Engine
        PRIVATE Engine_Graphics
)

target_compile_features(Blockgame PRIVATE cxx_std_23)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Textures
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Copy shaders to the build directory.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})