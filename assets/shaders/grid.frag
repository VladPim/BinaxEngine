#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform vec3 viewPos;
uniform float maxDistance = 50.0;   // дальность видимости сетки

void main() {
    float dist = length(viewPos - WorldPos);
    // Затухание: чем дальше, тем прозрачнее
    float alpha = clamp(1.0 - dist / maxDistance, 0.0, 0.6);
    
    // Рисуем только тонкие линии (используем step от дробной части)
    float gridSize = 1.0;
    vec2 fracXZ = fract(WorldPos.xz / gridSize);
    float lineX = step(0.98, fracXZ.x) + step(fracXZ.x, 0.02);
    float lineZ = step(0.98, fracXZ.y) + step(fracXZ.y, 0.02);
    float isLine = clamp(lineX + lineZ, 0.0, 1.0);
    
    // Центральные оси ярче
    float axisX = (abs(WorldPos.x) < 0.05) ? 1.0 : 0.0;
    float axisZ = (abs(WorldPos.z) < 0.05) ? 1.0 : 0.0;
    float brightness = max(isLine, max(axisX, axisZ));
    
    // Итоговый цвет – белый, с прозрачностью, зависящей от расстояния
    FragColor = vec4(1.0, 1.0, 1.0, alpha * brightness);
}