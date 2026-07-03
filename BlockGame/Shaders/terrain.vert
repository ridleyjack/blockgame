#version 450
#extension GL_EXT_buffer_reference: require

layout (buffer_reference, std430) readonly buffer CameraBuffer {
    mat4 view;
    mat4 projection;
};

layout (push_constant) uniform Push {
    CameraBuffer camera;
} pushData;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in uint inTexIdx;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragTexCoord;
layout (location = 2) out flat uint fragTexIdx;

void main() {
    gl_Position = pushData.camera.projection * pushData.camera.view * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragTexIdx = inTexIdx;
}
