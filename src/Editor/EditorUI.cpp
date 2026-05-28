// GLEW must be included first
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Editor/EditorUI.h"
#include "EditorTheme.h"
#include "Scene/SceneManager.h"
#include "Scene/GameObject.h"
#include "Graphics/Primitives.h"
#include "Graphics/Material.h"
#include "Graphics/Skybox.h"
#include "Graphics/Model.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuizmo.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <fstream>
#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <unordered_map>  // для snap-настроек
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <filesystem>
#include "Physics/PhysicsWorld.h"
#include <nlohmann/json.hpp>

static std::string GetFileNameWithoutExt(const std::string& path) {
    std::filesystem::path p(path);
    return p.stem().string();
}

extern bool mouseCaptured;
extern bool firstMouse;

// Вспомогательные функции для key-value файлов (snap)
static std::unordered_map<std::string, std::string> LoadKeyValueFile(const std::string& filename) {
    std::unordered_map<std::string, std::string> result;
    std::ifstream file(filename);
    if (!file.is_open()) return result;
    std::string line;
    while (std::getline(file, line)) {
        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            result[key] = value;
        }
    }
    return result;
}

static void SaveKeyValueFile(const std::string& filename, const std::unordered_map<std::string, std::string>& data) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    for (const auto& [key, value] : data) {
        file << key << "=" << value << "\n";
    }
}

EditorUI::EditorUI() {
    std::cout << "EditorUI created" << std::endl;
}

EditorUI::~EditorUI() {
    Shutdown();
}

bool EditorUI::Initialize(GLFWwindow* window, SceneManager* sceneManager) {
    m_Window = window;
    m_SceneManager = sceneManager;

    IMGUI_CHECKVERSION();
    m_ImGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_ImGuiContext);

    // ========== ЗАГРУЗКА ПОЛЬЗОВАТЕЛЬСКОГО ШРИФТА ==========
    ImGuiIO& io = ImGui::GetIO();
    const char* fontPath = "resources/fonts/EngineFont.ttf";
    FILE* testFile = fopen(fontPath, "rb");
    if (testFile) {
        fclose(testFile);
        // Загружаем шрифт размером 18px
        io.Fonts->AddFontFromFileTTF(fontPath, 18.0f);
        io.FontDefault = io.Fonts->Fonts.back();
        std::cout << "Custom font loaded: " << fontPath << std::endl;
    } else {
        io.Fonts->AddFontDefault();
        std::cerr << "Custom font not found, using default." << std::endl;
    }
    // ========================================================

    // Настройка сохранения окон (как у вас было)
    std::filesystem::create_directories("saves");
    io.IniFilename = "saves/editor.uiconf";
    m_FirstLaunch = !std::filesystem::exists("saves/editor.uiconf");

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Инициализация бэкендов
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::cerr << "Failed to initialize ImGui GLFW backend" << std::endl;
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 130")) {
        std::cerr << "Failed to initialize ImGui OpenGL backend" << std::endl;
        return false;
    }

    SetupImGuiStyle();
    m_Theme.ApplyToImGui();

    LoadEditorSettings();  // загружает всё и сразу применяет vsync/seamless

    std::cout << "EditorUI initialized successfully" << std::endl;
    return true;
}

void EditorUI::Shutdown() {
    if (m_ImGuiContext) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(m_ImGuiContext);
        m_ImGuiContext = nullptr;
    }
}

void EditorUI::SetupImGuiStyle() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Обнуляем все скругления
    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.ChildRounding = 0.0f;      // для дочерних окон
    style.PopupRounding = 0.0f;      // для всплывающих окон
    style.ScrollbarRounding = 0.0f;
    style.TabRounding = 0.0f;        // для вкладок (если используете)
    
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
}

void EditorUI::BeginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
    ImGuizmo::SetImGuiContext(m_ImGuiContext);
}

void EditorUI::EndFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorUI::SetViewProjection(const glm::mat4& view, const glm::mat4& projection) {
    m_ViewMatrix = view;
    m_ProjectionMatrix = projection;
}

void EditorUI::Render() {
    // Дефолтные позиции при первом запуске
    if (m_FirstLaunch) {
        ImGui::SetNextWindowPos(ImVec2(1, 26), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(126, 887), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(1333, 146), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(249, 461), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(1339, 617), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(244, 263), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(130, 21), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(134, 97), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(135, 777), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(244, 125), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(1333, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(246, 106), ImGuiCond_FirstUseEver);
        m_FirstLaunch = false;
    }

    DrawMainMenuBar();
    DrawHierarchy();
    DrawInspector();
    DrawSceneView();
    DrawSceneSettings();
    DrawContentBrowser();
    DrawThemeEditor();
    DrawSkyboxSettings();
    DrawShadowsSettings();

    if (m_ShowAboutPopup) {
        ImGui::OpenPopup("About");
        m_ShowAboutPopup = false;
    }

    if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Binax Engine v0.3.0");
        ImGui::Text("A simple 3D game engine with normal mapping");
        ImGui::Separator();
        ImGui::Text("Created with OpenGL, GLFW and Dear ImGui");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void EditorUI::DrawMainMenuBar() {

    if (ImGui::MenuItem("Import Model...")) {
    std::string path = OpenFileDialog("*.obj;*.fbx;*.dae;*.blend;*.3ds;*.stl");
    if (!path.empty()) {
        auto model = std::make_shared<Model>(path);
        if (model->IsLoaded()) {
            const auto& meshes = model->GetMeshes();
            if (meshes.size() == 1) {
                // Один меш: создаём объект с именем файла
                auto obj = m_SceneManager->CreateGameObject(GetFileNameWithoutExt(path));
                obj->SetMesh(meshes[0]);
                if (meshes[0]->GetMaterial()) obj->SetMaterial(meshes[0]->GetMaterial());
            } else {
                // Несколько мешей: создаём корневой объект и дочерние
                auto root = m_SceneManager->CreateGameObject(GetFileNameWithoutExt(path));
                for (const auto& mesh : meshes) {
                    auto child = m_SceneManager->CreateGameObject(mesh->GetName());
                    child->SetMesh(mesh);
                    if (mesh->GetMaterial()) child->SetMaterial(mesh->GetMaterial());
                    root->AddChild(child);
                }
            }
        } else {
            std::cerr << "Failed to load model: " << path << std::endl;
        }
    }
}

if (ImGui::BeginMenu("Physics")) {
    bool simActive = PhysicsWorld::GetInstance().IsSimulating();
    if (ImGui::MenuItem("Active Physics", nullptr, &simActive)) {
        m_SceneManager->SetPhysicsActive(simActive);
    }
    if (ImGui::MenuItem("Return")) {
        m_SceneManager->ResetPhysics();
    }
    ImGui::EndMenu();
}

    if (ImGui::BeginMainMenuBar()) {
        m_MenuBarHeight = ImGui::GetWindowHeight();

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                if (m_SceneManager) m_SceneManager->SaveScene("scene.binax");
            }
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                if (m_SceneManager) m_SceneManager->DuplicateSelectedObject();
            }
            if (ImGui::MenuItem("Delete", "Del")) {
                if (m_SceneManager) {
                    auto selected = m_SceneManager->GetSelectedObject();
                    if (selected) m_SceneManager->DeleteGameObject(selected.get());
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Skybox Settings")) {
                m_ShowSkyboxSettings = true;
            }
            if (ImGui::MenuItem("Shadows Settings")) {
                m_ShowShadowsSettings = true;
            }
            ImGui::EndMenu();
        }

        // Внутри DrawMainMenuBar, секция меню View:
if (ImGui::BeginMenu("View")) {
    ImGui::MenuItem("Wireframe Mode", "", &m_Settings.wireframe_mode);
    ImGui::MenuItem("Show Grid", "", &m_Settings.grid_enabled);
    ImGui::MenuItem("Show Gizmo", "", &m_Settings.show_gizmo);
    ImGui::Separator();
    ImGui::MenuItem("Theme Editor", "", &m_ShowThemeEditor);
    ImGui::Separator();
    if (ImGui::MenuItem("VSync", "", &m_Settings.vsync)) {
        glfwSwapInterval(m_Settings.vsync ? 1 : 0);
        SaveEditorSettings();   // вместо SaveVSyncSettings
    }
    ImGui::EndMenu();
}

        ImGui::SameLine(ImGui::GetWindowWidth() - 350);
        DrawGizmoToolbar();
        ImGui::SameLine();

        if (mouseCaptured) {
            if (ImGui::Button("Release Mouse (ESC)")) {
                mouseCaptured = false;
                firstMouse = true;
                glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        } else {
            if (ImGui::Button("Capture Mouse")) {
                mouseCaptured = true;
                firstMouse = true;
                glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }

        ImGui::EndMainMenuBar();
    }
}

void EditorUI::DrawGizmoToolbar() {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));

    if (m_CurrentGizmoOperation == ImGuizmo::TRANSLATE) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
    }
    if (ImGui::Button("T", ImVec2(30, 25))) {
        m_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
    }
    if (m_CurrentGizmoOperation == ImGuizmo::TRANSLATE) {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();

    if (m_CurrentGizmoOperation == ImGuizmo::ROTATE) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
    }
    if (ImGui::Button("R", ImVec2(30, 25))) {
        m_CurrentGizmoOperation = ImGuizmo::ROTATE;
    }
    if (m_CurrentGizmoOperation == ImGuizmo::ROTATE) {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();

    if (m_CurrentGizmoOperation == ImGuizmo::SCALE) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
    }
    if (ImGui::Button("S", ImVec2(30, 25))) {
        m_CurrentGizmoOperation = ImGuizmo::SCALE;
    }
    if (m_CurrentGizmoOperation == ImGuizmo::SCALE) {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();

    const char* modeText = (m_CurrentGizmoMode == ImGuizmo::WORLD) ? "World" : "Local";
    if (ImGui::Button(modeText, ImVec2(50, 25))) {
        m_CurrentGizmoMode = (m_CurrentGizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
    }

    ImGui::SameLine();

    // Snap controls
bool oldUseSnap = m_Settings.useSnap;
    ImGui::Checkbox("Snap", &m_Settings.useSnap);
    if (oldUseSnap != m_Settings.useSnap) SaveEditorSettings();   // заменено

    if (m_Settings.useSnap) {
        ImGui::SameLine();
        ImGui::PushItemWidth(60);
        if (m_CurrentGizmoOperation == ImGuizmo::TRANSLATE) {
            float oldVal = m_Settings.snapTranslation;
            ImGui::DragFloat("##SnapT", &m_Settings.snapTranslation, 0.01f, 0.01f, 10.0f, "%.2f");
            if (oldVal != m_Settings.snapTranslation) SaveEditorSettings();
        } else if (m_CurrentGizmoOperation == ImGuizmo::ROTATE) {
            float oldVal = m_Settings.snapRotation;
            ImGui::DragFloat("##SnapR", &m_Settings.snapRotation, 0.1f, 0.1f, 180.0f, "%.1f°");
            if (oldVal != m_Settings.snapRotation) SaveEditorSettings();
        } else if (m_CurrentGizmoOperation == ImGuizmo::SCALE) {
            float oldVal = m_Settings.snapScale;
            ImGui::DragFloat("##SnapS", &m_Settings.snapScale, 0.01f, 0.01f, 10.0f, "%.2f");
            if (oldVal != m_Settings.snapScale) SaveEditorSettings();
        }
        ImGui::PopItemWidth();
    }

    ImGui::PopStyleColor(3);
}

void EditorUI::DrawHierarchy() {
    if (ImGui::Begin("Hierarchy")) {
        ImGui::Text("Scene Objects");
        ImGui::Separator();

        if (m_SceneManager) {
            int id = 0;
            for (const auto& obj : m_SceneManager->GetObjects()) {
                if (obj->GetParent() == nullptr) {  // только корни
                    DrawObjectTreeNode(obj, id);
                }
            }

            ImGui::Separator();
            if (ImGui::Button("+ Add Object", ImVec2(-1, 0))) {
                ImGui::OpenPopup("AddObjectPopup");
            }

            if (ImGui::BeginPopup("AddObjectPopup")) {
    if (ImGui::MenuItem("Empty Object")) {
        m_SceneManager->CreateGameObject("Empty");
    }
    if (ImGui::MenuItem("Directional Light")) {
    if (m_SceneManager && !m_SceneManager->HasDirectionalLight()) {
        auto light = m_SceneManager->CreateGameObject("DirectionalLight");
        light->SetPosition(glm::vec3(2.0f, 4.0f, 2.0f));
        light->SetColor(glm::vec3(1.0f, 1.0f, 1.0f));
        light->SetScale(glm::vec3(0.3f));
        light->SetMesh(Primitives::CreateCube());
        m_Settings.light_pos = light->GetPosition();
        m_Settings.light_color = glm::vec3(1.0f);
        m_Settings.light_intensity = 1.0f;
    } else if (m_SceneManager && m_SceneManager->HasDirectionalLight()) {
        std::cerr << "Warning: Only one Directional Light allowed!" << std::endl;
    }
}

if (ImGui::MenuItem("Point Light")) {
    if (m_SceneManager) {
        auto light = m_SceneManager->CreateGameObject("PointLight");
        light->SetLightType(LT_POINT);
        light->SetPosition(glm::vec3(2.0f, 2.0f, 2.0f));
        light->SetLightColor(glm::vec3(1.0f, 1.0f, 0.8f));
        light->SetLightIntensity(2.0f);
        light->SetLightRange(8.0f);
        // Для визуализации можно добавить меш-сферу (опционально)
        light->SetMesh(Primitives::CreateSphere(16));
        light->SetScale(glm::vec3(0.2f));
    }
}
if (ImGui::MenuItem("Spot Light")) {
    if (m_SceneManager) {
        auto light = m_SceneManager->CreateGameObject("SpotLight");
        light->SetLightType(LT_SPOT);
        light->SetPosition(glm::vec3(3.0f, 3.0f, 0.0f));
        light->SetLightDirection(glm::vec3(-1.0f, -1.0f, 0.0f));
        light->SetLightColor(glm::vec3(1.0f, 0.5f, 0.2f));
        light->SetLightIntensity(3.0f);
        light->SetLightRange(12.0f);
        light->SetLightAngle(30.0f); // градусов
        light->SetMesh(Primitives::CreateCone(16));
        light->SetScale(glm::vec3(0.3f));
    }
}

if (ImGui::MenuItem("Base Fog")) {
    auto fog = m_SceneManager->CreateGameObject("Base Fog");
    fog->SetIsFog(true);
    fog->SetFogEnabled(false);   // по умолчанию выключен
    fog->SetColor(glm::vec3(0.5f, 0.6f, 0.7f)); // цвет для иконки в иерархии
    // Не даём меш, чтобы не было визуального представления (только компонент)
}

if (ImGui::MenuItem("Camera")) {
    auto camera = m_SceneManager->CreateGameObject("Camera");
    camera->SetIsCamera(true);
    camera->SetPosition(glm::vec3(0.0f, 2.0f, 5.0f));
    camera->SetRotation(glm::vec3(0.0f, -90.0f, 0.0f)); // чтобы смотрела в -Z
    camera->SetCameraFOV(45.0f);
    camera->SetCameraNear(0.1f);
    camera->SetCameraFar(100.0f);
    // НЕ вызываем SetMesh
}

    ImGui::Separator();
if (ImGui::MenuItem("Cube")) {
    auto cube = m_SceneManager->CreateGameObject("Cube");
    cube->SetMesh(Primitives::CreateCube());  
    cube->SetColor(glm::vec3(0.8f, 0.3f, 0.2f));
}
if (ImGui::MenuItem("Sphere")) {
    auto sphere = m_SceneManager->CreateGameObject("Sphere");
    sphere->SetMesh(Primitives::CreateSphere()); 
    sphere->SetColor(glm::vec3(0.3f, 0.8f, 0.2f));
}
    ImGui::EndPopup();
}
        }
    }
    ImGui::End();
}

void EditorUI::DrawObjectTreeNode(std::shared_ptr<GameObject> obj, int& id) {
    ImGui::PushID(id++);
    bool isSelected = (obj == m_SceneManager->GetSelectedObject());

    const char* icon = "[]";
    if (obj->GetName().find("Light") != std::string::npos) icon = "[L]";
    else if (obj->GetName().find("Camera") != std::string::npos) icon = "[C]";
    else if (obj->IsFog()) icon = "[F]";

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (isSelected) nodeFlags |= ImGuiTreeNodeFlags_Selected;
    if (obj->GetChildren().empty()) {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool nodeOpen = ImGui::TreeNodeEx((std::string(icon) + " " + obj->GetName()).c_str(), nodeFlags);

    if (ImGui::IsItemClicked()) {
        m_SceneManager->SetSelectedObject(obj);
    }

    // Drag & Drop — перетаскивание объекта
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoDisableHover)) {
        ImGui::SetDragDropPayload("GAME_OBJECT", obj.get(), sizeof(GameObject*));
        ImGui::Text("%s", obj->GetName().c_str());
        ImGui::EndDragDropSource();
    }

    // Drag & Drop — принятие объекта (сделать ребёнком)
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT")) {
            GameObject* droppedObjRaw = *(GameObject**)payload->Data;
            auto droppedObj = m_SceneManager->FindGameObjectByPtr(droppedObjRaw);
            if (droppedObj && droppedObj != obj) {
                // Защита от циклов
                bool isDescendant = false;
                auto parent = obj->GetParent();
                while (parent) {
                    if (parent == droppedObj) { isDescendant = true; break; }
                    parent = parent->GetParent();
                }
                if (!isDescendant) {
                    droppedObj->SetParent(obj, true);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Контекстное меню
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
            m_SceneManager->DuplicateSelectedObject();
        }
        if (ImGui::MenuItem("Delete", "Del")) {
            m_SceneManager->DeleteGameObject(obj.get());
        }
        ImGui::Separator();
        if (obj->GetParent() != nullptr) {
            if (ImGui::MenuItem("Make Root")) {
                obj->Unparent();
            }
        }
        if (ImGui::MenuItem("Make child of selected")) {
            auto selected = m_SceneManager->GetSelectedObject();
            if (selected && selected != obj) {
                obj->SetParent(selected, true);
            }
        }
        ImGui::EndPopup();
    }

    if (nodeOpen) {
        for (const auto& child : obj->GetChildren()) {
            DrawObjectTreeNode(child, id);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}


void EditorUI::DrawInspector() {
    if (ImGui::Begin("Inspector")) {
        auto selected = m_SceneManager ? m_SceneManager->GetSelectedObject() : nullptr;
        if (selected) {
            ImGui::Text("%s", selected->GetName().c_str());
            ImGui::Separator();

            // Transform – всегда
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawTransformControls(selected);
            }

            // Если это камера – только Camera секция
            if (selected->IsCamera()) {
                if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
                    float fov = selected->GetCameraFOV();
                    if (ImGui::SliderFloat("Field of View", &fov, 1.0f, 120.0f))
                        selected->SetCameraFOV(fov);
                    float nearPlane = selected->GetCameraNear();
                    if (ImGui::DragFloat("Near Plane", &nearPlane, 0.01f, 0.01f, 10.0f))
                        selected->SetCameraNear(nearPlane);
                    float farPlane = selected->GetCameraFar();
                    if (ImGui::DragFloat("Far Plane", &farPlane, 0.1f, 10.0f, 1000.0f))
                        selected->SetCameraFar(farPlane);

                    bool culling = selected->GetFrustumCulling();
                    if (ImGui::Checkbox("Frustum Culling", &culling)) {
                        selected->SetFrustumCulling(culling);
                    }

                    if (ImGui::Button("Switch to this camera"))
                        m_SceneManager->SetActiveCamera(selected);
                }
            } else {   // <-- теперь else правильно привязан к if (selected->IsCamera())
                // Для не-камер – все остальные секции
                if (ImGui::CollapsingHeader("Appearance", ImGuiTreeNodeFlags_DefaultOpen)) {
                    glm::vec3 color = selected->GetColor();
                    float colorArr[3] = { color.x, color.y, color.z };
                    if (ImGui::ColorEdit3("Color", colorArr)) {
                        selected->SetColor(glm::vec3(colorArr[0], colorArr[1], colorArr[2]));
                        if (selected->GetName() == "DirectionalLight") {
                            m_Settings.light_color = glm::vec3(colorArr[0], colorArr[1], colorArr[2]);
                        }
                    }
                    bool visible = selected->IsVisible();
                    if (ImGui::Checkbox("Visible", &visible)) {
                        selected->SetVisible(visible);
                    }
                }

                // Light section
                if (selected->GetLightType() != LT_NONE) {
                    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                        glm::vec3 col = selected->GetLightColor();
                        if (ImGui::ColorEdit3("Color", glm::value_ptr(col))) {
                            selected->SetLightColor(col);
                        }
                        float intensity = selected->GetLightIntensity();
                        if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 10.0f)) {
                            selected->SetLightIntensity(intensity);
                        }
                        int lightType = selected->GetLightType();
                        if (lightType == LT_POINT || lightType == LT_SPOT) {
                            float range = selected->GetLightRange();
                            if (ImGui::SliderFloat("Range", &range, 1.0f, 30.0f)) {
                                selected->SetLightRange(range);
                            }
                        }
                        if (lightType == LT_SPOT) {
                            float angle = selected->GetLightAngleDeg();
                            if (ImGui::SliderFloat("Angle (deg)", &angle, 5.0f, 120.0f)) {
                                selected->SetLightAngle(angle);
                            }
                            glm::vec3 dir = selected->GetLightDirection();
                            
                            if (ImGui::DragFloat3("Direction", glm::value_ptr(dir), 0.05f, -1.0f, 1.0f)) {
                                selected->SetLightDirection(glm::normalize(dir));
                            }
                            // после настройки угла и направления
ImGui::Separator();
ImGui::Text("Fake Volumetric Cone");
bool shaft = selected->GetShaftEnabled();
if (ImGui::Checkbox("Enable Shaft", &shaft)) {
    selected->SetShaftEnabled(shaft);
}
if (shaft) {
    float intensity = selected->GetShaftIntensity();
    if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.5f))
        selected->SetShaftIntensity(intensity);
    float softness = selected->GetShaftSoftness();
    if (ImGui::SliderFloat("Softness", &softness, 0.2f, 2.0f))
        selected->SetShaftSoftness(softness);
    float density = selected->GetShaftDensity();
if (ImGui::SliderFloat("Density", &density, 0.0f, 1.0f))
    selected->SetShaftDensity(density);
}

                        }
                        if (lightType == LT_DIRECTIONAL) {
                            glm::vec3 dir = selected->GetLightDirection();
                            if (ImGui::DragFloat3("Direction", glm::value_ptr(dir), 0.05f, -1.0f, 1.0f)) {
                                selected->SetLightDirection(glm::normalize(dir));
                            }
                        }
                    }
                }

                if (selected->IsFog()) {
                    if (ImGui::CollapsingHeader("Fog Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                        bool enabled = selected->GetFogEnabled();
                        if (ImGui::Checkbox("Enable Fog", &enabled)) {
                            selected->SetFogEnabled(enabled);
                        }
                        if (enabled) {
                            const char* fogTypes[] = { "Linear", "Exponential", "Exponential Squared" };
                            int currentType = selected->GetFogType() - 1;
                            if (ImGui::Combo("Type", &currentType, fogTypes, 3)) {
                                selected->SetFogType(currentType + 1);
                            }
                            glm::vec3 color = selected->GetFogColor();
                            if (ImGui::ColorEdit3("Color", glm::value_ptr(color))) {
                                selected->SetFogColor(color);
                            }
                            if (selected->GetFogType() == 1) {
                                float start = selected->GetFogLinearStart();
                                float end = selected->GetFogLinearEnd();
                                if (ImGui::DragFloat("Start", &start, 0.5f, 0.0f, 200.0f))
                                    selected->SetFogLinearStart(start);
                                if (ImGui::DragFloat("End", &end, 0.5f, 0.0f, 500.0f))
                                    selected->SetFogLinearEnd(end);
                            } else {
                                float density = selected->GetFogDensity();
                                if (ImGui::DragFloat("Density", &density, 0.002f, 0.0f, 0.5f))
                                    selected->SetFogDensity(density);
                            }
                        }
                    }
                }

                // Material, Mesh, Rendering, Components sections...
                if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
                    auto material = selected->GetMaterial();
                    if (!material) {
                        ImGui::Text("No material assigned.");
                        if (ImGui::Button("Create Material")) {
                            material = std::make_shared<Material>();
                            selected->SetMaterial(material);
                        }
                    } else {
                        DrawMaterialControls(selected);
                        ImGui::SameLine();
                        if (ImGui::Button("Remove Material")) {
                            selected->SetMaterial(nullptr);
                        }
                    }
                }

                if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
                    const char* meshNames[] = { "Cube", "Sphere", "Cylinder", "Cone", "Pyramid", "Plane" };
                    static int currentMesh = -1;
                    if (ImGui::Combo("Mesh Type", &currentMesh, meshNames, IM_ARRAYSIZE(meshNames))) {
                        std::shared_ptr<Mesh> newMesh;
                        switch (currentMesh) {
    case 0: newMesh = Primitives::CreateCube(); break;
    case 1: newMesh = Primitives::CreateSphere(); break;
    case 2: newMesh = Primitives::CreateCylinder(); break;
    case 3: newMesh = Primitives::CreateCone(); break;
                            case 4: newMesh = Primitives::CreatePyramid(); break;
                            case 5: newMesh = Primitives::CreatePlane(); break;
                        }
                        if (newMesh) selected->SetMesh(newMesh);
                    }
                }

                if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
                    bool cast = selected->CastShadows();
                    bool receive = selected->ReceiveShadows();
                    if (ImGui::Checkbox("Cast Shadows", &cast)) selected->SetCastShadows(cast);
                    if (ImGui::Checkbox("Receive Shadows", &receive)) selected->SetReceiveShadows(receive);
                }

                if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen)) {
                    bool canAddComponents = !selected->IsCamera() && selected->GetLightType() == LT_NONE;
                    if (!canAddComponents) {
                        ImGui::TextDisabled("Components not available for lights or cameras");
                    } else {
                        bool hasPhysics = selected->HasRigidBody() || selected->GetColliderType() != COLLIDER_NONE;
                        if (hasPhysics) {
                            DrawPhysicsComponents(selected);
                            if (ImGui::Button("Remove Physics")) {
                                selected->RemoveRigidBody();
                                selected->SetColliderType(COLLIDER_NONE);
                            }
                        } else {
                            if (ImGui::Button("Add Component")) {
                                ImGui::OpenPopup("add_component_popup");
                            }
                            if (ImGui::BeginPopup("add_component_popup")) {
                                if (ImGui::MenuItem("Physics")) {
                                    if (selected->CanHavePhysics()) {
                                        selected->SetColliderType(COLLIDER_BOX);
                                        selected->AddRigidBody(1.0f);
                                        selected->SaveInitialTransform();
                                        PhysicsWorld::GetInstance().RegisterGameObject(selected.get());
                                    } else {
                                        ImGui::OpenPopup("physics_error");
                                    }
                                }
                                ImGui::EndPopup();
                            }
                            if (ImGui::BeginPopupModal("physics_error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                                ImGui::Text("Cannot add physics to object with parent or children!");
                                if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
                                ImGui::EndPopup();
                            }
                        }
                    }
                }
            } // конец else (не-камера)
        } // конец if (selected)
        ImGui::End(); // перемещено внутрь блока Begin
    } // конец if (ImGui::Begin)
}

void EditorUI::DrawMaterialControls(std::shared_ptr<GameObject> obj) {
    auto material = obj->GetMaterial();
    if (!material) return;

    ImGui::Text("Material: %s", obj->GetName().c_str());
    ImGui::Separator();

    // ---- Текстуры ----
    ImGui::Text("Textures");
    
    // Diffuse
    ImGui::Text("Diffuse: %s", material->HasDiffuse() ? "Loaded" : "None");
    ImGui::SameLine();
    if (ImGui::Button("Load##Diffuse")) {
        std::string path = OpenFileDialog("*.jpg;*.png;*.bmp");
        if (!path.empty()) material->LoadDiffuseTexture(path);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear##Diffuse") && material->HasDiffuse()) {
        material->ClearDiffuse();
    }

    // Normal
    ImGui::Text("Normal: %s", material->HasNormal() ? "Loaded" : "None");
    ImGui::SameLine();
    if (ImGui::Button("Load##Normal")) {
        std::string path = OpenFileDialog("*.jpg;*.png;*.bmp");
        if (!path.empty()) material->LoadNormalTexture(path);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear##Normal") && material->HasNormal()) {
        material->ClearNormal();
    }

    // Roughness
    ImGui::Text("Roughness: %s", material->HasRoughness() ? "Loaded" : "None");
    ImGui::SameLine();
    if (ImGui::Button("Load##Roughness")) {
        std::string path = OpenFileDialog("*.jpg;*.png;*.bmp");
        if (!path.empty()) material->LoadRoughnessTexture(path);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear##Roughness") && material->HasRoughness()) {
        material->ClearRoughness();
    }

    // Metallic
    ImGui::Text("Metallic: %s", material->HasMetallic() ? "Loaded" : "None");
    ImGui::SameLine();
    if (ImGui::Button("Load##Metallic")) {
        std::string path = OpenFileDialog("*.jpg;*.png;*.bmp");
        if (!path.empty()) material->LoadMetallicTexture(path);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear##Metallic") && material->HasMetallic()) {
        material->ClearMetallic();
    }

    // AO
    ImGui::Text("AO: %s", material->HasAO() ? "Loaded" : "None");
    ImGui::SameLine();
    if (ImGui::Button("Load##AO")) {
        std::string path = OpenFileDialog("*.jpg;*.png;*.bmp");
        if (!path.empty()) material->LoadAOTexture(path);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear##AO") && material->HasAO()) {
        material->ClearAO();
    }

    ImGui::Separator();

    // ---- Параметры ----
    ImGui::Text("Material Parameters");
    
    if (material->HasNormal()) {
        float strength = material->normalStrength;
        if (ImGui::SliderFloat("Normal Strength", &strength, 0.0f, 2.0f))
            material->normalStrength = strength;
    }

    float uvScale[2] = { material->uvScale.x, material->uvScale.y };
    if (ImGui::DragFloat2("UV Scale", uvScale, 0.1f, 0.1f, 10.0f))
        material->uvScale = glm::vec2(uvScale[0], uvScale[1]);

    bool worldUV = material->useWorldUV;
    if (ImGui::Checkbox("Use World UV", &worldUV))
        material->useWorldUV = worldUV;

    if (!material->HasMetallic()) {
        float metal = material->metallic;
        if (ImGui::SliderFloat("Metallic", &metal, 0.0f, 1.0f))
            material->metallic = metal;
    }

    if (!material->HasRoughness()) {
        float rough = material->roughness;
        if (ImGui::SliderFloat("Roughness", &rough, 0.0f, 1.0f))
            material->roughness = rough;
    }

    // ---- Emission ----
    ImGui::Separator();
    ImGui::Text("Emission");
    float emissionCol[3] = { material->emissionColor.x, material->emissionColor.y, material->emissionColor.z };
    float emissionInt = material->emissionIntensity;
    if (ImGui::ColorEdit3("Color", emissionCol))
        material->emissionColor = glm::vec3(emissionCol[0], emissionCol[1], emissionCol[2]);
    if (ImGui::SliderFloat("Intensity", &emissionInt, 0.0f, 5.0f))
        material->emissionIntensity = emissionInt;

    // ---- Reflections checkbox (без сворачиваемого заголовка) ----
    ImGui::Separator();
    bool reflectionsEnabled = material->enableReflections;
    if (ImGui::Checkbox("Enable Environment Reflections", &reflectionsEnabled)) {
        material->enableReflections = reflectionsEnabled;
    }

    // ---- Material File Operations ----
    ImGui::Separator();
    ImGui::Text("Material File Operations");

    if (ImGui::Button("Save Material As...")) {
        std::string path = SaveFileDialog("Material Files\0*.binaxmat\0", "binaxmat");
        if (!path.empty()) {
            if (material->SaveToFile(path))
                std::cout << "Material saved to " << path << std::endl;
            else
                std::cerr << "Failed to save material" << std::endl;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Material")) {
        std::string path = OpenFileDialog("Material Files\0*.binaxmat\0");
        if (!path.empty()) {
            auto newMaterial = std::make_shared<Material>();
            if (newMaterial->LoadFromFile(path)) {
                obj->SetMaterial(newMaterial);
                std::cout << "Material loaded from " << path << std::endl;
            } else {
                std::cerr << "Failed to load material" << std::endl;
            }
        }
    }
}

void EditorUI::DrawTransformControls(std::shared_ptr<GameObject> obj) {
    glm::vec3 pos = obj->GetPosition();
    glm::vec3 rot = obj->GetRotation();
    glm::vec3 scale = obj->GetScale();

    float posArr[3] = { pos.x, pos.y, pos.z };
    float rotArr[3] = { rot.x, rot.y, rot.z };
    float scaleArr[3] = { scale.x, scale.y, scale.z };

    if (ImGui::DragFloat3("Position", posArr, 0.1f)) {
        obj->SetPosition(glm::vec3(posArr[0], posArr[1], posArr[2]));
        if (obj->GetName() == "DirectionalLight") {
            m_Settings.light_pos = glm::vec3(posArr[0], posArr[1], posArr[2]);
        }
    }
    if (ImGui::DragFloat3("Rotation", rotArr, 1.0f, -180.0f, 180.0f)) {
        obj->SetRotation(glm::vec3(rotArr[0], rotArr[1], rotArr[2]));
    }
    if (ImGui::DragFloat3("Scale", scaleArr, 0.1f, 0.001f, 10.0f)) {
        obj->SetScale(glm::vec3(scaleArr[0], scaleArr[1], scaleArr[2]));
    }

    // Кнопки Reset
    ImGui::SameLine();
    if (ImGui::Button("R##Pos")) obj->SetPosition(glm::vec3(0.0f));
    ImGui::SameLine();
    if (ImGui::Button("R##Rot")) obj->SetRotation(glm::vec3(0.0f));
    ImGui::SameLine();
    if (ImGui::Button("R##Scale")) obj->SetScale(glm::vec3(1.0f));

    // Проверка на не-uniform масштаб
    glm::vec3 currentScale = obj->GetScale();
    if (currentScale.x != currentScale.y || currentScale.x != currentScale.z || currentScale.y != currentScale.z) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
        ImGui::TextWrapped("Warning: Non-uniform scale may cause shadow artifacts!");
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        if (ImGui::Button("Fix Scale")) {
            // Используем std::max с явным указанием типа, чтобы избежать конфликта с макросом max
            float maxScale = (std::max)({currentScale.x, currentScale.y, currentScale.z});
            obj->SetScale(glm::vec3(maxScale));
        }
    }
}

void EditorUI::DrawSceneView() {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove;
    if (ImGui::Begin("Scene View", nullptr, flags)) {
        m_ViewportPos = ImGui::GetCursorScreenPos();
        m_ViewportSize = ImGui::GetContentRegionAvail();
        m_ViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
        m_ViewportFocused = ImGui::IsWindowFocused();

        m_GizmoActive = false;

        auto selected = GetSelectedObject();
        if (selected && m_Settings.show_gizmo && m_ViewportHovered) {
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(m_ViewportPos.x, m_ViewportPos.y, m_ViewportSize.x, m_ViewportSize.y);

            glm::mat4 transform = selected->GetTransformMatrix();
            float snap[3] = {0,0,0};
            if (m_Settings.useSnap) {
                if (m_CurrentGizmoOperation == ImGuizmo::TRANSLATE)
                    snap[0]=snap[1]=snap[2]=m_Settings.snapTranslation;
                else if (m_CurrentGizmoOperation == ImGuizmo::ROTATE)
                    snap[0]=snap[1]=snap[2]=m_Settings.snapRotation;
                else if (m_CurrentGizmoOperation == ImGuizmo::SCALE)
                    snap[0]=snap[1]=snap[2]=m_Settings.snapScale;
            }

            if (ImGuizmo::Manipulate(glm::value_ptr(m_ViewMatrix), glm::value_ptr(m_ProjectionMatrix),
                                     m_CurrentGizmoOperation, m_CurrentGizmoMode,
                                     glm::value_ptr(transform), nullptr, m_Settings.useSnap ? snap : nullptr))
            {
                glm::mat4 finalMatrix = transform;
                if (selected->GetParent()) {
                    glm::mat4 parentWorld = selected->GetParent()->GetTransformMatrix();
                    finalMatrix = glm::inverse(parentWorld) * transform;
                }
                glm::vec3 scale, pos, skew;
                glm::quat rot;
                glm::vec4 persp;
                glm::decompose(finalMatrix, scale, rot, pos, skew, persp);
                selected->SetPosition(pos);
                selected->SetRotation(glm::degrees(glm::eulerAngles(rot)));
                selected->SetScale(scale);
                m_GizmoActive = true;
            }
            else if (ImGuizmo::IsUsing()) {
                m_GizmoActive = true;
            }
        } // <-- закрываем if (selected && ...)

        // Текст в углу...
        ImGui::SetCursorPos(ImVec2(10,10));
        ImGui::TextColored(ImVec4(1,1,1,0.7f), "Scene View");
        ImGui::SetCursorPos(ImVec2(10,30));
        ImGui::TextColored(ImVec4(1,1,1,0.5f), "%.0fx%.0f", m_ViewportSize.x, m_ViewportSize.y);
    } // <-- закрываем if (ImGui::Begin(...))
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void EditorUI::DrawSceneSettings() {
    if (ImGui::Begin("Scene Settings")) {
        if (ImGui::CollapsingHeader("Environment")) {
            ImGui::ColorEdit3("Background", m_Settings.bg_color);
            ImGui::Checkbox("Show Grid", &m_Settings.grid_enabled);
            ImGui::Checkbox("Wireframe Mode", &m_Settings.wireframe_mode);
            ImGui::Checkbox("Show Gizmo", &m_Settings.show_gizmo);
            ImGui::SliderFloat("Ambient Strength", &m_Settings.ambientStrength, 0.0f, 0.5f);
        }

        // VSync с сохранением
        if (ImGui::Checkbox("VSync", &m_Settings.vsync)) {
            glfwSwapInterval(m_Settings.vsync ? 1 : 0);
            SaveEditorSettings();   // добавлено
        }

        ImGui::Separator();
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    }
    ImGui::End();
}

void EditorUI::DrawContentBrowser() {
    if (ImGui::Begin("Content Browser")) {
        ImGui::Text("Project Files");
        ImGui::Separator();
        ImGui::Text("Assets/");
    }
    ImGui::End();
}

void EditorUI::DrawMaterialSettings() {
    if (ImGui::Begin("Material Settings")) {
        ImGui::SliderFloat("Shininess", &m_Settings.shininess, 1.0f, 256.0f);
        ImGui::SliderFloat("Metallic", &m_Settings.metallic, 0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &m_Settings.roughness, 0.0f, 1.0f);
    }
    ImGui::End();
}

void EditorUI::HandleShortcuts() {
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
        if (m_SceneManager) m_SceneManager->SaveScene("scene.binax");
    }

    // Shift + A — фокус на окне Hierarchy
if (ImGui::IsKeyPressed(ImGuiKey_A) && ImGui::GetIO().KeyShift) {
    ImGui::SetWindowFocus("Hierarchy");
}

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false)) {
        if (m_SceneManager) m_SceneManager->DuplicateSelectedObject();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
    if (m_SceneManager) {
        auto selected = m_SceneManager->GetSelectedObject();
        if (selected) {
            if (selected->GetName() == "DirectionalLight") {
                m_Settings.light_intensity = 0.0f;
                m_Settings.light_color = glm::vec3(0.0f);
            }
            m_SceneManager->DeleteGameObject(selected.get());
        }
    }
}

    if (!io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_W, false)) {
        m_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
    }
    if (!io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_E, false)) {
        m_CurrentGizmoOperation = ImGuizmo::ROTATE;
    }
    if (!io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_R, false)) {
        m_CurrentGizmoOperation = ImGuizmo::SCALE;
    }
    if (!io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X, false)) {
        m_CurrentGizmoMode = (m_CurrentGizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_G, false)) {
        m_Settings.grid_enabled = !m_Settings.grid_enabled;
    }
}

void EditorUI::DrawThemeEditor() {
    if (!m_ShowThemeEditor) return;

    ImGui::Begin("Theme Editor", &m_ShowThemeEditor);

    ImGui::Text("Customize UI Colors");

    float accent[3] = { m_Theme.accent.x, m_Theme.accent.y, m_Theme.accent.z };
    float bg[3] = { m_Theme.background.x, m_Theme.background.y, m_Theme.background.z };
    float text[3] = { m_Theme.text.x, m_Theme.text.y, m_Theme.text.z };
    float header[3] = { m_Theme.header.x, m_Theme.header.y, m_Theme.header.z };
    float button[3] = { m_Theme.button.x, m_Theme.button.y, m_Theme.button.z };
    float frameBg[3] = { m_Theme.frameBg.x, m_Theme.frameBg.y, m_Theme.frameBg.z };

    if (ImGui::ColorEdit3("Accent Color", accent)) {
        m_Theme.accent = glm::vec3(accent[0], accent[1], accent[2]);
        m_Theme.button = m_Theme.accent * 0.5f;
        m_Theme.buttonHovered = m_Theme.accent * 0.8f;
        m_Theme.buttonActive = m_Theme.accent;
        m_Theme.ApplyToImGui();
    }
    if (ImGui::ColorEdit3("Background", bg)) {
        m_Theme.background = glm::vec3(bg[0], bg[1], bg[2]);
        m_Theme.ApplyToImGui();
    }
    if (ImGui::ColorEdit3("Text Color", text)) {
        m_Theme.text = glm::vec3(text[0], text[1], text[2]);
        m_Theme.ApplyToImGui();
    }
    if (ImGui::ColorEdit3("Header Color", header)) {
        m_Theme.header = glm::vec3(header[0], header[1], header[2]);
        m_Theme.ApplyToImGui();
    }
    if (ImGui::ColorEdit3("Frame Background", frameBg)) {
        m_Theme.frameBg = glm::vec3(frameBg[0], frameBg[1], frameBg[2]);
        m_Theme.ApplyToImGui();
    }

    ImGui::Separator();
    if (ImGui::Button("Reset to Default")) {
        m_Theme = EditorTheme();
        m_Theme.ApplyToImGui();
    }

    ImGui::End();
}

void EditorUI::DrawShadowsSettings() {
    if (!m_ShowShadowsSettings) return;

    ImGui::Begin("Shadows Settings", &m_ShowShadowsSettings);

    ImGui::Checkbox("Enable Shadows", &m_Settings.shadows_enabled);
    ImGui::SliderFloat("Shadow Bias", &m_Settings.shadow_bias, 0.0f, 0.01f, "%.5f");

    ImGui::Separator();
    ImGui::Text("Quality");
    // Для размера карты пока не будем менять динамически, просто показываем
    ImGui::SliderInt("Shadow Map Size", &m_Settings.shadowMapSize, 512, 4096, "%d");
    // Увеличиваем диапазон softness до 5
    ImGui::SliderFloat("Softness", &m_Settings.shadowSoftness, 0.0f, 5.0f, "%.2f");
    const char* sampleModes[] = { "4 samples", "9 samples" };
    int currentSamples = (m_Settings.shadowSamples == 4) ? 0 : 1;
    if (ImGui::Combo("PCF Samples", &currentSamples, sampleModes, 2)) {
        m_Settings.shadowSamples = (currentSamples == 0) ? 4 : 9;
    }

    ImGui::End();
}

void EditorUI::DrawSkyboxSettings() {
    if (!m_ShowSkyboxSettings) return;

    ImGui::Begin("Skybox Settings", &m_ShowSkyboxSettings);

    const char* sideNames[6] = { "Right (+X)", "Left (-X)", "Top (+Y)", "Bottom (-Y)", "Front (+Z)", "Back (-Z)" };
    for (int i = 0; i < 6; i++) {
        ImGui::Text("%s:", sideNames[i]);
        ImGui::SameLine();
        ImGui::Text("%s", m_SkyboxPaths[i].c_str());
        ImGui::SameLine();
        if (ImGui::Button(("Load##" + std::to_string(i)).c_str())) {
            std::string path = OpenFileDialog("*.png;*.jpg;*.bmp");
            if (!path.empty()) {
                m_SkyboxPaths[i] = path;
            }
        }
    }

    if (ImGui::Button("Apply Skybox")) {
        if (m_Skybox) {
            m_Skybox->Load(
                m_SkyboxPaths[0], m_SkyboxPaths[1],
                m_SkyboxPaths[2], m_SkyboxPaths[3],
                m_SkyboxPaths[4], m_SkyboxPaths[5]
            );
        }
    }

    bool seamlessChanged = false;
    if (ImGui::Checkbox("Remove joints (seamless)", &m_Settings.skyboxSeamless)) {
        seamlessChanged = true;
    }
    if (seamlessChanged) {
        if (m_Settings.skyboxSeamless)
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        else
            glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        SaveEditorSettings();   // заменено (было SaveSnapSettings)
    }

    ImGui::End();
}

std::shared_ptr<GameObject> EditorUI::GetSelectedObject() const {
    return m_SceneManager ? m_SceneManager->GetSelectedObject() : nullptr;
}
void EditorUI::SetSelectedObject(std::shared_ptr<GameObject> obj) {
    if (m_SceneManager) m_SceneManager->SetSelectedObject(obj);
}

std::string EditorUI::OpenFileDialog(const char* filter) {
    char filename[MAX_PATH] = {};
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = glfwGetWin32Window(m_Window);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    }
    return "";
}

std::string EditorUI::SaveFileDialog(const char* filter, const char* defaultExt) {
    char filename[MAX_PATH] = {};
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = glfwGetWin32Window(m_Window);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    
    if (GetSaveFileNameA(&ofn)) {
        return std::string(filename);
    }
    return "";
}

void EditorUI::DrawPhysicsComponents(std::shared_ptr<GameObject> selected) {
    ImGui::Text("Physics Components");
    ImGui::Separator();

    // RigidBody
    if (!selected->HasRigidBody()) {
        if (ImGui::Button("Add RigidBody")) {
            selected->AddRigidBody(1.0f);
            selected->SaveInitialTransform();
            PhysicsWorld::GetInstance().RegisterGameObject(selected.get());
        }
    } else {
        ImGui::Text("RigidBody (mass = %.1f)", selected->GetMass());
        if (ImGui::Button("Remove RigidBody"))
            selected->RemoveRigidBody();

        float mass = selected->GetMass();
        if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.0f, 100.0f)) {
            selected->SetMass(mass);
            selected->RemoveRigidBody();
            selected->AddRigidBody(mass);
        }

        ImGui::Separator();
        ImGui::Text("Material Properties");
        float friction = selected->GetFriction();
        if (ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f))
            selected->SetFriction(friction);
        float restitution = selected->GetRestitution();
        if (ImGui::SliderFloat("Restitution", &restitution, 0.0f, 1.0f))
            selected->SetRestitution(restitution);
        float rollingFriction = selected->GetRollingFriction();
        if (ImGui::SliderFloat("Rolling Friction", &rollingFriction, 0.0f, 1.0f))
            selected->SetRollingFriction(rollingFriction);

        ImGui::Separator();
        ImGui::Text("Damping");
        float linearDamping = selected->GetLinearDamping();
        if (ImGui::SliderFloat("Linear Damping", &linearDamping, 0.0f, 1.0f))
            selected->SetLinearDamping(linearDamping);
        float angularDamping = selected->GetAngularDamping();
        if (ImGui::SliderFloat("Angular Damping", &angularDamping, 0.0f, 1.0f))
            selected->SetAngularDamping(angularDamping);
    }

    ImGui::Separator();
    ImGui::Text("Collider");
    int currentCollider = (int)selected->GetColliderType();
    const char* colliderItems[] = { "None", "Box", "Sphere", "Capsule" };
    if (ImGui::Combo("Type", &currentCollider, colliderItems, 4)) {
        if (currentCollider != COLLIDER_NONE && !selected->CanHavePhysics()) {
            ImGui::OpenPopup("collider_error");
        } else {
            selected->SetColliderType((ColliderType)currentCollider);
            if (selected->HasRigidBody()) {
                float mass = selected->GetMass();
                selected->RemoveRigidBody();
                selected->AddRigidBody(mass);
            }
        }
    }
    if (ImGui::BeginPopupModal("collider_error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Cannot add collider to object with parent/children!");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void EditorUI::LoadEditorSettings() {
    auto data = LoadKeyValueFile("saves/editorscene.settingscfg");
    if (data.empty()) return;

    try {
        // Snap
        if (data.count("useSnap")) m_Settings.useSnap = (std::stoi(data["useSnap"]) != 0);
        if (data.count("snapTranslation")) m_Settings.snapTranslation = std::stof(data["snapTranslation"]);
        if (data.count("snapRotation")) m_Settings.snapRotation = std::stof(data["snapRotation"]);
        if (data.count("snapScale")) m_Settings.snapScale = std::stof(data["snapScale"]);
        if (data.count("vsync")) m_Settings.vsync = (std::stoi(data["vsync"]) != 0);
        if (data.count("skyboxSeamless")) m_Settings.skyboxSeamless = (std::stoi(data["skyboxSeamless"]) != 0);
    } catch (...) {
        std::cerr << "Failed to parse editorscene.settingscfg" << std::endl;
    }

    // Применяем настройки, требующие немедленного действия
    glfwSwapInterval(m_Settings.vsync ? 1 : 0);
    if (m_Settings.skyboxSeamless)
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    else
        glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void EditorUI::SaveEditorSettings() {
    std::unordered_map<std::string, std::string> data;
    data["useSnap"] = std::to_string(m_Settings.useSnap ? 1 : 0);
    data["snapTranslation"] = std::to_string(m_Settings.snapTranslation);
    data["snapRotation"] = std::to_string(m_Settings.snapRotation);
    data["snapScale"] = std::to_string(m_Settings.snapScale);
    data["vsync"] = std::to_string(m_Settings.vsync ? 1 : 0);
    data["skyboxSeamless"] = std::to_string(m_Settings.skyboxSeamless ? 1 : 0);
    SaveKeyValueFile("saves/editorscene.settingscfg", data);
} 
