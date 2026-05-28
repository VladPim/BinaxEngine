#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform float intensity;      // общая яркость (0..2)
uniform float softness;       // «мягкость» по оси (чем больше, тем дальше затухание)
uniform float density;        // плотность дымки (0..1) – новое!
uniform vec3 lightColor;

// Простая псевдо-случайная функция для шума (опционально)
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

void main() {
    // TexCoords: y = 1 у источника, 0 у основания; x = 0..1 по окружности (0..2pi)
    float distFromTip = 1.0 - TexCoords.y;   // 0 у источника, 1 у основания
    
    // 1. Градиент вдоль луча (затухание с расстоянием)
    float axial = pow(distFromTip, 1.0 / max(softness, 0.1)) * intensity;
    
    // 2. Градиент от центра к краям (дымка ярче по краям? Нет, обычно центр ярче)
    //    Сделаем эффект «объёма» – центр ярче, края прозрачнее.
    float radial = 1.0 - abs(TexCoords.x - 0.5) * 2.0;   // 1 в центре, 0 на краях
    radial = pow(radial, 1.5);                           // чуть сужаем
    
    // 3. Итоговая альфа: комбинация осевого и радиального градиента
    float alpha = axial * (0.7 + 0.3 * radial);
    
    // 4. Применяем плотность дымки (общий множитель)
    alpha *= density;
    
    // 5. Добавляем случайный шум (очень слабый, для «мерцания» дымки)
    //    Раскомментировать, если хотите эффект живого тумана.
    // float noise = random(gl_FragCoord.xy * 0.01) * 0.1;
    // alpha += noise;
    
    // 6. Ограничиваем максимальную прозрачность (чтобы не слепило внутри конуса)
    alpha = min(alpha, 0.6);
    
    FragColor = vec4(lightColor, alpha);
}