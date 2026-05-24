#include "Scene/SceneManager.h"
#include "Graphics/Primitives.h"
#include "Graphics/Material.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Physics/PhysicsWorld.h"
#include "Graphics/Shader.h"
#include <glm/gtx/quaternion.hpp>

SceneManager::SceneManager() {
    m_Initialized = false;
}

SceneManager::~SceneManager() {
    m_Objects.clear();
    m_SelectedObject.reset();
}

void SceneManager::Initialize() {
    
    if (m_Initialized) return;
    std::cout << "Initializing SceneManager..." << std::endl;

    m_GridMesh = Primitives::CreateGrid(500);   // 500x500 юнитов

    
    m_ShaftShader.Load("assets/shaders/shaft.vert", "assets/shaders/shaft.frag");

    auto light = CreateGameObject("DirectionalLight");
    light->SetLightType(LT_DIRECTIONAL);        
    light->SetLightColor(glm::vec3(1.0f, 1.0f, 1.0f));
    light->SetLightIntensity(1.0f);
    light->SetLightDirection(glm::vec3(-1.0f, -1.0f, 0.0f)); // направление света
    light->SetPosition(glm::vec3(2.0f, 4.0f, 2.0f)); // позиция не важна для directional, но для визуализации ок
    light->SetColor(glm::vec3(1.0f, 1.0f, 1.0f));
    light->SetScale(glm::vec3(0.3f));
    light->SetMesh(Primitives::CreateCube());
    light->SetLightDirection(glm::normalize(glm::vec3(-1.0f, -2.0f, -1.0f)));

    SetSelectedObject(light);
    m_Initialized = true;
    std::cout << "SceneManager initialized with " << m_Objects.size() << " objects" << std::endl;

    auto mainCamera = CreateGameObject("Scene Camera");
    mainCamera->SetIsCamera(true);
    mainCamera->SetPosition(glm::vec3(0.0f, 2.0f, 5.0f));
    mainCamera->SetRotation(glm::vec3(0.0f, -90.0f, 0.0f)); // важно: yaw = -90, pitch = 0
    mainCamera->SetCameraFOV(45.0f);
    mainCamera->SetCameraNear(0.1f);
    mainCamera->SetCameraFar(100.0f);
    SetActiveCamera(mainCamera);

    m_Initialized = true;
}

void SceneManager::SetSelectedObject(std::shared_ptr<GameObject> object) {
    m_SelectedObject = object;
}

std::shared_ptr<GameObject> SceneManager::CreateGameObject(const std::string& name) {
    auto obj = std::make_shared<GameObject>(name);
    m_Objects.push_back(obj);
    return obj;
}

void SceneManager::DeleteGameObject(GameObject* object) {
    if (!object) return;

    // Если удаляемый объект является выбранным, сбрасываем выделение
    if (m_SelectedObject.get() == object) {
        m_SelectedObject.reset();
    }

    // Если объект имеет родителя, открепляем его
    if (object->GetParent()) {
        object->GetParent()->RemoveChild(object);
    }

    // Рекурсивно удаляем всех детей (они уже будут удалены из списков родителей)
    for (auto& child : object->GetChildren()) {
        DeleteGameObject(child.get());
    }

    // Удаляем из общего списка
    auto it = std::find_if(m_Objects.begin(), m_Objects.end(),
        [object](const std::shared_ptr<GameObject>& ptr) { return ptr.get() == object; });
    if (it != m_Objects.end()) {
        m_Objects.erase(it);
    }
}

void SceneManager::DuplicateSelectedObject() {
    if (!m_SelectedObject) return;
    // Запрещаем дублировать DirectionalLight (можно и другие типы разрешить)
    if (m_SelectedObject->GetName() == "DirectionalLight") {
        std::cerr << "Cannot duplicate Directional Light" << std::endl;
        return;
    }
    auto newObj = CreateGameObject(m_SelectedObject->GetName() + " (Copy)");
    newObj->SetMesh(m_SelectedObject->GetMesh());
    newObj->SetPosition(m_SelectedObject->GetPosition() + glm::vec3(1.0f, 0.0f, 0.0f));
    newObj->SetRotation(m_SelectedObject->GetRotation());
    newObj->SetScale(m_SelectedObject->GetScale());
    newObj->SetColor(m_SelectedObject->GetColor());
    if (m_SelectedObject->GetMaterial()) {
        newObj->SetMaterial(m_SelectedObject->GetMaterial());
    }
    // Копируем параметры света
    newObj->SetLightType(m_SelectedObject->GetLightType());
    newObj->SetLightColor(m_SelectedObject->GetLightColor());
    newObj->SetLightIntensity(m_SelectedObject->GetLightIntensity());
    newObj->SetLightRange(m_SelectedObject->GetLightRange());
    newObj->SetLightAngle(m_SelectedObject->GetLightAngleDeg());
    newObj->SetLightDirection(m_SelectedObject->GetLightDirection());

    SetSelectedObject(newObj);
}

void SceneManager::Update(float deltaTime) {}

void SceneManager::Render(Shader& shader) {
    if (!m_Initialized) return;
    for (const auto& obj : m_Objects) {
        if (obj->IsVisible()) {
            obj->Draw(shader);
        }
    }
}

void SceneManager::RenderDepth(Shader& depthShader) {
    if (!m_Initialized) return;
    for (const auto& obj : m_Objects) {
        if (obj->IsVisible() && obj->GetMesh() && obj->CastShadows()) {
            glm::mat4 model = obj->GetTransformMatrix();
            depthShader.SetMat4("model", glm::value_ptr(model));
            obj->GetMesh()->Draw();
        }
    }
}

void SceneManager::RenderOutline(Shader& outlineShader, const glm::mat4& view, const glm::mat4& projection, 
                                 const glm::vec3& color, int mode, float pointSize, float fillAlpha) {
    if (!m_SelectedObject || !m_SelectedObject->IsVisible() || !m_SelectedObject->GetMesh()) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);

    outlineShader.Use();
    outlineShader.SetMat4("view", glm::value_ptr(view));
    outlineShader.SetMat4("projection", glm::value_ptr(projection));
    outlineShader.SetVec3("outlineColor", color.x, color.y, color.z); // Добавляем

    glm::mat4 transform = m_SelectedObject->GetTransformMatrix();
    glm::vec3 scale = m_SelectedObject->GetScale();
    glm::vec3 biggerScale = scale * 1.05f;
    glm::mat4 outlineTransform = glm::scale(transform, biggerScale / scale);
    outlineShader.SetMat4("model", glm::value_ptr(outlineTransform));

    if (mode == 0) { // Wireframe
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        m_SelectedObject->GetMesh()->Draw();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else if (mode == 1) { // Vertices
        glPointSize(pointSize);
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        m_SelectedObject->GetMesh()->Draw();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else if (mode == 2) { // Fill
        outlineShader.SetFloat("alpha", fillAlpha);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        m_SelectedObject->GetMesh()->Draw();
    }

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

bool SceneManager::HasDirectionalLight() const {
    for (const auto& obj : m_Objects) {
        if (obj->GetName() == "DirectionalLight") {
            return true;
        }
    }
    return false;
}

void SceneManager::SetActiveCamera(std::shared_ptr<GameObject> camera) {
    if (camera && camera->IsCamera()) {
        m_ActiveCamera = camera;
        // Извлекаем текущий поворот камеры для управления от первого лица
        glm::vec3 rot = m_ActiveCamera->GetRotation();
        m_CameraYaw = rot.y;
        m_CameraPitch = rot.x;
        // Убедимся, что углы в допустимых пределах
        if (m_CameraPitch > 89.0f) m_CameraPitch = 89.0f;
        if (m_CameraPitch < -89.0f) m_CameraPitch = -89.0f;
    }
}

void SceneManager::MoveActiveCamera(float forwardBack, float leftRight, float upDown, float speed) {
    if (!m_ActiveCamera) return;

    // Получаем матрицу поворота камеры (без переноса)
    glm::mat4 transform = m_ActiveCamera->GetTransformMatrix();
    glm::vec3 forward = glm::normalize(glm::vec3(transform[2]));   // ось Z (куда смотрит камера)
    glm::vec3 right   = glm::normalize(glm::vec3(transform[0]));   // ось X
    glm::vec3 up      = glm::normalize(glm::vec3(transform[1]));   // ось Y

    // В OpenGL камера по умолчанию смотрит в -Z, но матрица модели камеры хранит её ориентацию.
    // Чтобы двигаться вперёд по направлению взгляда, нужно использовать -forward, если камера смотрит в -Z.
    // Однако проще: используем forward как есть, а при необходимости инвертируем знак.
    // Проверим: если объект камеры повёрнут так, что его локальная ось Z направлена вперёд,
    // то forward = transform[2] уже указывает вперёд. Для единообразия сделаем так:

    glm::vec3 move = forward * forwardBack + right * leftRight + up * upDown;
    glm::vec3 newPos = m_ActiveCamera->GetPosition() + move * speed;
    m_ActiveCamera->SetPosition(newPos);
}

void SceneManager::RotateActiveCamera(float yawDelta, float pitchDelta) {
    if (!m_ActiveCamera) return;

    // Получаем текущий поворот в углах Эйлера (в градусах)
    glm::vec3 rot = m_ActiveCamera->GetRotation();
    float yaw = rot.y;
    float pitch = rot.x;

    // Применяем дельты (знаки уже скорректированы в mouse_callback)
    yaw += yawDelta;
    pitch += pitchDelta;

    // Ограничения
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Устанавливаем новый поворот
    m_ActiveCamera->SetRotation(glm::vec3(pitch, yaw, 0.0f));

    // Синхронизируем сохранённые углы для других нужд (если нужно)
    m_CameraYaw = yaw;
    m_CameraPitch = pitch;
}

void SceneManager::RenderGrid(Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
    if (!m_GridMesh) return;
    shader.Use();
    shader.SetMat4("view", glm::value_ptr(view));
    shader.SetMat4("projection", glm::value_ptr(projection));
    glm::mat4 model = glm::mat4(1.0f);
    shader.SetMat4("model", glm::value_ptr(model));
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_GridMesh->Draw();
    glDisable(GL_BLEND);
}

void SceneManager::InitializePhysics() {
    PhysicsWorld::GetInstance().Initialize();
    // Теперь мир существует, пересоздаём тела для всех объектов с коллайдером
    for (auto& obj : m_Objects) {
        if (obj->GetColliderType() != COLLIDER_NONE) {
            obj->UpdatePhysicsBody();  // создаст тело (статическое или динамическое)
        }
    }
}

void SceneManager::UpdatePhysics(float deltaTime) {
    PhysicsWorld::GetInstance().Update(deltaTime);
    for (auto& obj : m_Objects) {
        if (obj->HasRigidBody()) {
            obj->SyncTransformToPhysics();
            // отладочный вывод
            std::cout << "Syncing " << obj->GetName() << std::endl;
        }
    }
}

void SceneManager::SetPhysicsActive(bool active) {
    std::cout << "SceneManager::SetPhysicsActive(" << active << ")" << std::endl;
    PhysicsWorld::GetInstance().SetSimulationActive(active);
}

std::shared_ptr<GameObject> SceneManager::FindGameObjectByPtr(GameObject* ptr) {
    for (auto& obj : m_Objects) {
        if (obj.get() == ptr) return obj;
    }
    return nullptr;
}

std::shared_ptr<GameObject> SceneManager::GetActiveFog() const {
    for (const auto& obj : m_Objects) {
        if (obj->IsFog()) return obj;
    }
    return nullptr;
}

void SceneManager::SetFrustumCullingForActiveCamera(bool enabled) {
    if (m_ActiveCamera)
        m_ActiveCamera->SetFrustumCulling(enabled);
}

void SceneManager::RenderWithCulling(Shader& shader, const Frustum& frustum) {
    if (!m_Initialized) return;
    for (const auto& obj : m_Objects) {
        if (!obj->IsVisible()) continue;
        // Если объект — камера, всегда рисуем? нет, камера не рендерится
        if (obj->IsCamera()) continue;
        if (m_ActiveCamera && m_ActiveCamera->GetFrustumCulling()) {
            glm::vec3 minBB, maxBB;
            obj->CalculateAABB(minBB, maxBB);
            if (!frustum.IsAABBInside(minBB, maxBB))
                continue;
        }
        obj->Draw(shader);
    }
}

void SceneManager::ResetPhysics() {
    PhysicsWorld::GetInstance().ResetAllObjects();
}

void SceneManager::RegisterForPhysicsReset(GameObject* obj) {
    PhysicsWorld::GetInstance().RegisterGameObject(obj);
}

void SceneManager::RenderLightShafts(const glm::mat4& view, const glm::mat4& projection) {
    if (!m_Initialized) return;
    m_ShaftShader.Use();
    m_ShaftShader.SetMat4("view", glm::value_ptr(view));
    m_ShaftShader.SetMat4("projection", glm::value_ptr(projection));
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // аддитивное смешивание
    glDepthMask(GL_FALSE);
    
    for (const auto& obj : m_Objects) {
        if (obj->GetLightType() != LT_SPOT) continue;
        if (!obj->GetShaftEnabled()) continue;
        if (!obj->GetShaftMesh()) obj->UpdateShaftMesh();
        auto mesh = obj->GetShaftMesh();
        if (!mesh) continue;
        
        // Матрица модели: источник света (позиция), направление света
        glm::vec3 pos = obj->GetWorldPosition();
        glm::vec3 dir = obj->GetLightDirection();
        glm::vec3 defaultDir(0.0f, 0.0f, -1.0f);
        glm::quat rot = glm::rotation(defaultDir, glm::normalize(dir));
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
        model = model * glm::mat4_cast(rot);
        
        m_ShaftShader.SetMat4("model", glm::value_ptr(model));
        m_ShaftShader.SetFloat("intensity", obj->GetShaftIntensity());
        m_ShaftShader.SetFloat("softness", obj->GetShaftSoftness());
        m_ShaftShader.SetFloat("density", obj->GetShaftDensity());
        m_ShaftShader.SetVec3("lightColor", obj->GetLightColor().x, obj->GetLightColor().y, obj->GetLightColor().z);
        
        mesh->Draw();
    }
    
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void SceneManager::SaveScene(const std::string& filename) {
    std::cout << "Saving scene to: " << filename << std::endl;
}

void SceneManager::LoadScene(const std::string& filename) {
    std::cout << "Loading scene from: " << filename << std::endl;
}
