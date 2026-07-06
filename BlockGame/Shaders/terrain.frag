#version 450

layout (set = 0, binding = 0) uniform sampler2DArray texSampler;

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexCoord;
layout (location = 2) in flat uint fragTexIdx;

layout (location = 3) in vec3 fragWorldPos;
layout (location = 4) in vec3 fragCameraWorldPos;
layout (location = 5) in vec3 fragFogColor;
layout (location = 6) in float fragFogDensity;
layout (location = 7) in float fragFogStart;

layout (location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, vec3(fragTexCoord, fragTexIdx));
    vec3 baseColor = texColor.rgb;

    float d = length(fragWorldPos - fragCameraWorldPos);

    float fogStart = fragFogStart;
    float fogDistance = max(d - fogStart, 0.0);

    float fogAmount = 1.0 - exp(-fogDistance * fragFogDensity);
    fogAmount = clamp(fogAmount, 0.0, 1.0);

    vec3 color = mix(baseColor, fragFogColor, fogAmount);
    outColor = vec4(color, texColor.a);
}