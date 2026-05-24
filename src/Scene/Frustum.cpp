#include "Frustum.h"
#include <glm/gtc/matrix_access.hpp>

void Frustum::Update(const glm::mat4& viewProj) {
    // Extract planes from view-projection matrix (row-major OpenGL style)
    // Left   = row3 + row0
    planes[0] = glm::row(viewProj, 3) + glm::row(viewProj, 0);
    // Right  = row3 - row0
    planes[1] = glm::row(viewProj, 3) - glm::row(viewProj, 0);
    // Bottom = row3 + row1
    planes[2] = glm::row(viewProj, 3) + glm::row(viewProj, 1);
    // Top    = row3 - row1
    planes[3] = glm::row(viewProj, 3) - glm::row(viewProj, 1);
    // Near   = row3 + row2
    planes[4] = glm::row(viewProj, 3) + glm::row(viewProj, 2);
    // Far    = row3 - row2
    planes[5] = glm::row(viewProj, 3) - glm::row(viewProj, 2);

    for (auto& p : planes) {
        float len = glm::length(glm::vec3(p));
        p /= len;
    }
}

float Frustum::PlaneDistance(const glm::vec4& plane, const glm::vec3& point) const {
    return glm::dot(glm::vec3(plane), point) + plane.w;
}

bool Frustum::IsSphereInside(const glm::vec3& center, float radius) const {
    for (const auto& p : planes) {
        if (PlaneDistance(p, center) < -radius)
            return false;
    }
    return true;
}

bool Frustum::IsAABBInside(const glm::vec3& min, const glm::vec3& max) const {
    // Положительная вершина (самая дальняя по нормали плоскости)
    for (const auto& p : planes) {
        glm::vec3 positiveVertex;
        positiveVertex.x = (p.x > 0) ? max.x : min.x;
        positiveVertex.y = (p.y > 0) ? max.y : min.y;
        positiveVertex.z = (p.z > 0) ? max.z : min.z;
        if (PlaneDistance(p, positiveVertex) < 0)
            return false;
    }
    return true;
}