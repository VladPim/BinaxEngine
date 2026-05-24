#pragma once
#include <vector>
#include <string>
#include <memory>
#include <GL/glew.h>
#include <glm/glm.hpp>

class Material;

struct Vertex {
    float Position[3];
    float Normal[3];
    float TexCoords[2];
    float Tangent[3];
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices,
         const std::vector<unsigned int>& indices,
         const std::string& diffusePath = "",
         const std::string& normalPath = "");
    ~Mesh();

    void Draw() const;
    void SetMaterial(std::shared_ptr<Material> material) { m_Material = material; }
    std::shared_ptr<Material> GetMaterial() const { return m_Material; }
    void SetName(const std::string& name) { m_Name = name; }
    std::string GetName() const { return m_Name; }
    const std::vector<Vertex>& GetVertices() const { return m_Vertices; }

private:
    GLuint VAO = 0, VBO = 0, EBO = 0;
    size_t m_IndexCount = 0;
    GLuint m_DiffuseTexture = 0;
    GLuint m_NormalTexture = 0;
    std::shared_ptr<Material> m_Material;
    std::string m_Name;
    std::vector<Vertex> m_Vertices;

    void SetupMesh(const std::vector<Vertex>& vertices,
                   const std::vector<unsigned int>& indices);
    GLuint LoadTexture(const std::string& path);
};
