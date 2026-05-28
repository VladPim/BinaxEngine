#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture;
uniform sampler2D depthTexture;
uniform mat4 invProjection;
uniform mat4 invView;
uniform vec3 viewPos;
uniform vec3 fogColor;
uniform int fogType;
uniform float fogDensity;
uniform float fogStart;
uniform float fogEnd;
uniform bool fogEnabled;

// Восстановление мировых координат по глубине
vec3 WorldPosFromDepth(float depth, vec2 texCoord) {
    vec4 clipSpacePos = vec4(texCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpacePos = invProjection * clipSpacePos;
    viewSpacePos /= viewSpacePos.w;
    vec4 worldSpacePos = invView * viewSpacePos;
    return worldSpacePos.xyz;
}

void main() {
    vec3 color = texture(sceneTexture, TexCoords).rgb;
    float depth = texture(depthTexture, TexCoords).r;
    
    // Восстанавливаем мировые координаты пикселя
    vec3 worldPos = WorldPosFromDepth(depth, TexCoords);
    float dist = length(viewPos - worldPos);
    
    vec3 result = color;
if (fogEnabled) {
    float dist = length(viewPos - worldPos);
    float fogFactor = 0.0;
    if (fogType == 1) { // Linear
        fogFactor = (dist - fogStart) / (fogEnd - fogStart);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
    } else if (fogType == 2) { // Exponential
        fogFactor = 1.0 - exp(-fogDensity * dist);
    } else if (fogType == 3) { // Exponential Squared
        fogFactor = 1.0 - exp(-pow(fogDensity * dist, 2.0));
    }
    result = mix(color, fogColor, fogFactor);
}
FragColor = vec4(result, 1.0);
}