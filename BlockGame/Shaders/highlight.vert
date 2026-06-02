#version 450

layout (set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
} camera;

layout (push_constant) uniform Push {
    mat4 model;
} pushData;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in uint inTexIdx;

layout (location = 0) out vec3 fragColor;

void main()
{
    gl_Position = camera.projection * camera.view * pushData.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}