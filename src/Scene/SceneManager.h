#pragma once
#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "GameObject.h"
#include "Graphics/Shader.h"
#include "Scene/Frustum.h"

// ===== ТИПЫ ТУМАНА (глобально, чтобы использовать без SceneManager::) =====
enum FogType {
    FOG_NONE = 0,
    FOG_LINEAR,
    FOG_EXP,
    FOG_EXP_SQUARED
};

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    void Initialize();
    void Update(float deltaTime);
    void Render(Shader& shader);
    void RenderDepth(Shader& depthShader);
  
    std::shared_ptr<GameObject> CreateGameObject(const std::string& name);
    void DeleteGameObject(GameObject* object);
    void DuplicateSelectedObject();

    void SetSelectedObject(std::shared_ptr<GameObject> object);
    std::shared_ptr<GameObject> GetSelectedObject() const { return m_SelectedObject; }

    const std::vector<std::shared_ptr<GameObject>>& GetObjects() const { return m_Objects; }

    void SaveScene(const std::string& filename);
    void LoadScene(const std::string& filename);
    bool HasDirectionalLight() const;

    void SetActiveCamera(std::shared_ptr<GameObject> camera);
    std::shared_ptr<GameObject> GetActiveCamera() const { return m_ActiveCamera; }
    
    // Управление активной камерой (для редактора)
    void MoveActiveCamera(float forwardBack, float leftRight, float upDown, float speed);
    void RotateActiveCamera(float yawDelta, float pitchDelta);
    void UpdateActiveCamera(float deltaTime); // для плавности

    void RenderGrid(Shader& shader, const glm::mat4& view, const glm::mat4& projection);
    std::shared_ptr<Mesh> m_GridMesh;

    // Physics
    void InitializePhysics();
    void UpdatePhysics(float deltaTime);
    void SetPhysicsActive(bool active);
    void ResetPhysics();
    void RegisterForPhysicsReset(GameObject* obj);

    std::shared_ptr<GameObject> FindGameObjectByPtr(GameObject* ptr);

    // ===== НАСТРОЙКИ ТУМАНА =====
    struct FogSettings {
        bool enabled = false;
        int type = FOG_LINEAR;           // теперь FOG_LINEAR доступен глобально
        glm::vec3 color = glm::vec3(0.5f, 0.6f, 0.7f);
        float density = 0.04f;
        float linearStart = 10.0f;
        float linearEnd = 50.0f;
    };
    FogSettings& GetFogSettings() { return m_Fog; }
    std::shared_ptr<GameObject> GetActiveFog() const;
    void SetFrustumCullingForActiveCamera(bool enabled);
    void RenderWithCulling(Shader& shader, const Frustum& frustum);
    void RenderInstanced(Shader& shader, const Frustum* frustum = nullptr);
    void RenderLightShafts(const glm::mat4& view, const glm::mat4& projection);
    void RenderFrustumGizmos(Shader& shader, const glm::mat4& view, const glm::mat4& projection, 
                         const GameObject* activeCamera) const;
    void RenderLightGizmos(Shader& shader, const glm::mat4& view, const glm::mat4& projection) const;
    void RenderAudioGizmos(Shader& shader, const glm::mat4& view, const glm::mat4& projection) const;
    

private:
    bool m_Initialized;
    std::vector<std::shared_ptr<GameObject>> m_Objects;
    std::shared_ptr<GameObject> m_SelectedObject;
    std::shared_ptr<GameObject> m_ActiveCamera;
    float m_CameraYaw = -90.0f;
    float m_CameraPitch = 0.0f;
    FogSettings m_Fog;
    Shader m_ShaftShader;
};
