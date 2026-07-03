#version 450
#extension GL_EXT_buffer_reference: require

layout (buffer_reference, std430) readonly buffer CameraBuffer {
    mat4 view;
    mat4 projection;
};

layout (buffer_reference, std430) readonly buffer ModelBuffer {
    mat4 model;
};

layout (push_constant) uniform Push {
    CameraBuffer camera;
    ModelBuffer shaderData;
} pushData;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in uint inTexIdx;

layout (location = 0) out vec3 fragColor;

void main()
{
    gl_Position = pushData.camera.projection * pushData.camera.view * pushData.shaderData.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}
