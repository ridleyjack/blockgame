#version 450
#extension GL_EXT_buffer_reference: require

layout (buffer_reference, std430) readonly buffer CameraBuffer {
    mat4 view;
    mat4 projection;
};

layout (buffer_reference, std430) readonly buffer FogBuffer {
    vec3 CameraWorldPos;
    float FogDensity;
    vec3 FogColor;
    float FogStart;
};

layout (push_constant) uniform Push {
    CameraBuffer camera;
    FogBuffer fog;
} pushData;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in uint inTexIdx;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragTexCoord;
layout (location = 2) out flat uint fragTexIdx;

layout (location = 3) out vec3 fragWorldPos;
layout (location = 4) out vec3 fragCameraWorldPos;
layout (location = 5) out vec3 fragFogColor;
layout (location = 6) out float fragFogDensity;
layout (location = 7) out float fragFogStart;

void main() {
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragTexIdx = inTexIdx;

    fragWorldPos = inPosition;
    fragCameraWorldPos = pushData.fog.CameraWorldPos;
    fragFogColor = pushData.fog.FogColor;
    fragFogDensity = pushData.fog.FogDensity;
    fragFogStart = pushData.fog.FogStart;

    gl_Position = pushData.camera.projection * pushData.camera.view * vec4(inPosition, 1.0);
}
