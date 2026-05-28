#version 330 core
in vec3 TexCoords;
out vec4 FragColor;
uniform samplerCube skybox;

void main() {
    vec3 dir = normalize(TexCoords);
    // Сглаживание стыков: смешиваем выборки с небольшим смещением
    float eps = 0.002;
    vec3 sample0 = texture(skybox, dir).rgb;
    vec3 sample1 = texture(skybox, dir + vec3(eps, 0, 0)).rgb;
    vec3 sample2 = texture(skybox, dir - vec3(eps, 0, 0)).rgb;
    vec3 sample3 = texture(skybox, dir + vec3(0, eps, 0)).rgb;
    vec3 sample4 = texture(skybox, dir - vec3(0, eps, 0)).rgb;
    vec3 sample5 = texture(skybox, dir + vec3(0, 0, eps)).rgb;
    vec3 sample6 = texture(skybox, dir - vec3(0, 0, eps)).rgb;
    
    vec3 blended = (sample0 + sample1 + sample2 + sample3 + sample4 + sample5 + sample6) / 7.0;
    // Используем blended только на гранях (где производная велика)
    vec3 grad = abs(dFdx(dir)) + abs(dFdy(dir));
    float t = clamp(length(grad) * 8.0, 0.0, 1.0);
    vec3 finalColor = mix(sample0, blended, t);
    
    FragColor = vec4(finalColor, 1.0);
}
