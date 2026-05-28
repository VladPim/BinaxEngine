#pragma once
#include <string>
#include <GL/glew.h>

class Shader {
public:
    Shader() = default;
    ~Shader();

    bool Load(const std::string& vertexPath, const std::string& fragmentPath);
    void Use() const;

    // Uniform setters
    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec2(const std::string& name, float x, float y) const;
    void SetVec3(const std::string& name, float x, float y, float z) const;
    void SetMat4(const std::string& name, const float* matrix) const;

private:
    GLuint m_ID = 0;
    bool CheckCompileErrors(GLuint shader, const std::string& type);
    bool CreateDefaultShader();
};
