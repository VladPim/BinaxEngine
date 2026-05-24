#pragma once
#include <glm/glm.hpp>
#include <array>

class Frustum {
public:
    void Update(const glm::mat4& viewProj);
    bool IsSphereInside(const glm::vec3& center, float radius) const;
    bool IsAABBInside(const glm::vec3& min, const glm::vec3& max) const;

private:
    std::array<glm::vec4, 6> planes; // left, right, bottom, top, near, far
    float PlaneDistance(const glm::vec4& plane, const glm::vec3& point) const;
};