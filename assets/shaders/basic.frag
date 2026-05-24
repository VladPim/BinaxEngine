#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;
in vec4 FragPosLightSpace;

// === ТЕКСТУРЫ ===
uniform sampler2D diffuseTexture;    // альбедо
uniform sampler2D normalMap;         // карта нормалей
uniform sampler2D roughnessTexture;  // шероховатость (в канале R)
uniform sampler2D metallicTexture;   // металличность (в канале R)
uniform sampler2D aoTexture;         // ambient occlusion (в канале R)

uniform bool hasDiffuseTexture;
uniform bool hasNormalMap;
uniform bool hasRoughnessTexture;
uniform bool hasMetallicTexture;
uniform bool hasAOTexture;

uniform vec3 objectColor;            // если нет diffuse текстуры
uniform float metallic;              // глобальная металличность
uniform float roughness;             // глобальная шероховатость
uniform float normalStrength;        // сила нормалей (1.0 = полная)
uniform vec2 uvScale;
uniform bool useWorldUV;             // использовать мировые координаты для UV

// === НАСТРОЙКИ ОСВЕЩЕНИЯ ===
uniform vec3 viewPos;
uniform float ambientStrength;       // сила ambient освещения (глобальная)

// === ТЕНИ ===
uniform sampler2D shadowMap;
uniform bool shadowsEnabled;
uniform bool receiveShadows;
uniform float shadowBias;
uniform float shadowSoftness;        // мягкость тени (размер ядра PCF)
uniform int shadowSamples;           // 4 или 9 сэмплов

// === ЭМИССИЯ ===
uniform vec3 emissionColor;
uniform float emissionIntensity;

// === ТУМАН ===
uniform bool fogEnabled;
uniform int fogType;       // 0 = нет, 1 = linear, 2 = exp, 3 = exp2
uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogStart;
uniform float fogEnd;

// === ИСТОЧНИКИ СВЕТА ===
struct Light {
    int type;          // 0 = directional, 1 = point, 2 = spot
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float range;
    float angle;       // угол конуса спота (в радианах)
};
uniform Light lights[8];
uniform int numLights;

// ======================== ФУНКЦИИ PBR ========================

// Распределение GGX (Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.1415926535 * denom * denom;
    return nom / denom;
}

// Геометрическая функция Шлика для одного направления
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

// Геометрическая функция Смита (произведение для L и V)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// Fresnel-Schlick для металлов/диэлектриков
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// ======================== ФУНКЦИЯ ТЕНЕЙ ========================
float ShadowCalculation(vec4 fragPosLightSpace, float NdotL)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(shadowBias, 0.05 * (1.0 - NdotL));
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    if (shadowSamples == 4)
    {
        for (int x = -1; x <= 1; x += 2)
        {
            for (int y = -1; y <= 1; y += 2)
            {
                vec2 offset = vec2(x, y) * texelSize * shadowSoftness;
                float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
                shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 4.0;
    }
    else
    {
        for (int x = -1; x <= 1; x++)
        {
            for (int y = -1; y <= 1; y++)
            {
                vec2 offset = vec2(x, y) * texelSize * shadowSoftness;
                float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
                shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;
    }
    return shadow;
}

// ======================== MAIN ========================
void main()
{
    // ---------- UV и альбедо ----------
    vec2 uv = TexCoords * uvScale;
    vec3 albedo;
    
    if (useWorldUV)
    {
        // Трипланшное проецирование (мировые координаты)
        vec3 worldPos = FragPos;
        vec3 blend = abs(normalize(TBN[2]));
        blend = pow(blend, vec3(2.0));
        blend /= (blend.x + blend.y + blend.z);
        vec3 xaxis = texture(diffuseTexture, worldPos.yz * uvScale).rgb;
        vec3 yaxis = texture(diffuseTexture, worldPos.xz * uvScale).rgb;
        vec3 zaxis = texture(diffuseTexture, worldPos.xy * uvScale).rgb;
        albedo = xaxis * blend.x + yaxis * blend.y + zaxis * blend.z;
    }
    else
    {
        if (hasDiffuseTexture)
            albedo = texture(diffuseTexture, uv).rgb;
        else
            albedo = objectColor;
    }
    
    // ---------- Нормаль в мировом пространстве ----------
    vec3 geomNormal = normalize(TBN[2]);
    vec3 normal;
    if (hasNormalMap && !useWorldUV)
    {
        vec3 tangentNormal = texture(normalMap, uv).rgb * 2.0 - 1.0;
        tangentNormal.xy *= normalStrength;
        vec3 worldNormal = normalize(TBN * tangentNormal);
        normal = normalize(mix(geomNormal, worldNormal, clamp(normalStrength, 0.0, 1.0)));
    }
    else
    {
        normal = geomNormal;
    }
    
    // ---------- PBR параметры ----------
    float metallicVal = metallic;
    if (hasMetallicTexture)
        metallicVal = texture(metallicTexture, uv).r;
    
    float roughnessVal = roughness;
    if (hasRoughnessTexture)
        roughnessVal = texture(roughnessTexture, uv).r;
    // Ограничение roughness от 0.04 до 1.0 (чтобы не было артефактов)
    roughnessVal = clamp(roughnessVal, 0.04, 1.0);
    
    float aoVal = 1.0;
    if (hasAOTexture)
        aoVal = texture(aoTexture, uv).r;
    
    vec3 V = normalize(viewPos - FragPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallicVal);
    
    // Ambient (с AO)
    vec3 ambient = ambientStrength * albedo * aoVal;
    vec3 result = ambient;
    
    // ---------- Цикл по всем источникам света ----------
    for (int i = 0; i < numLights; i++)
    {
        Light light = lights[i];
        vec3 L;
        float attenuation = 1.0;
        
        // Типы света
        if (light.type == 0) // directional
        {
            L = normalize(-light.direction);
        }
        else if (light.type == 1) // point
        {
            vec3 delta = light.position - FragPos;
            float dist = length(delta);
            if (dist > light.range) continue;
            L = delta / dist;
            attenuation = 1.0 / (1.0 + dist * dist / (light.range * light.range));
        }
        else if (light.type == 2) // spot
        {
            vec3 delta = light.position - FragPos;
            float dist = length(delta);
            if (dist > light.range) continue;
            L = delta / dist;
            attenuation = 1.0 / (1.0 + dist * dist / (light.range * light.range));
            vec3 spotDir = normalize(light.direction);
            float cosTheta = dot(-L, spotDir);
            float spotEffect = smoothstep(cos(light.angle), cos(light.angle * 0.5), cosTheta);
            attenuation *= spotEffect;
        }
        
        float NdotL = max(dot(normal, L), 0.0);
        if (NdotL <= 0.0) continue;
        
        float NdotV = max(dot(normal, V), 0.0);
        vec3 H = normalize(V + L);
        float NdotH = max(dot(normal, H), 0.0);
        
        // PBR вычисления
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        float NDF = DistributionGGX(normal, H, roughnessVal);
        float G = GeometrySmith(normal, V, L, roughnessVal);
        
        vec3 specular = (NDF * G * F) / (4.0 * max(NdotV, 0.001) * NdotL);
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallicVal);
        vec3 diffuse = kD * albedo / 3.1415926535;
        
        vec3 radiance = light.color * light.intensity * attenuation;
        vec3 Lo = (diffuse + specular) * radiance * NdotL;
        
        // Тени только для directional света
        float shadow = 0.0;
        if (shadowsEnabled && receiveShadows && light.type == 0 && NdotL > 0.0)
        {
            shadow = ShadowCalculation(FragPosLightSpace, NdotL);
        }
        
        result += (1.0 - shadow) * Lo;
    }
    
    // ---------- Эмиссия ----------
    result += emissionColor * emissionIntensity;
    
    // ---------- Туман ----------
    if (fogEnabled)
    {
        float dist = length(viewPos - FragPos);
        float fogFactor = 0.0;
        if (fogType == 1) // Linear
        {
            fogFactor = (dist - fogStart) / (fogEnd - fogStart);
            fogFactor = clamp(fogFactor, 0.0, 1.0);
        }
        else if (fogType == 2) // Exponential
        {
            fogFactor = 1.0 - exp(-fogDensity * dist);
        }
        else if (fogType == 3) // Exponential squared
        {
            fogFactor = 1.0 - exp(-pow(fogDensity * dist, 2.0));
        }
        result = mix(result, fogColor, fogFactor);
    }
    
    FragColor = vec4(result, 1.0);
}
