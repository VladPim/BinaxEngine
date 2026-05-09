# 🎮 BinaxEngineTestVersion - Game Engine (v0.5.0 Beta)

[![CMake](https://img.shields.io/badge/CMake-3.15+-blue?style=flat-square&logo=cmake)](https://cmake.org/)
[![C++](https://img.shields.io/badge/C++-17-orange?style=flat-square&logo=cplusplus)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.6-red?style=flat-square&logo=opengl)](https://www.opengl.org/)
[![Bullet Physics](https://img.shields.io/badge/Bullet-3.25-brightgreen?style=flat-square)](https://pybullet.org/)
[![Assimp](https://img.shields.io/badge/Assimp-5.2-blue?style=flat-square)](https://assimp.org/)
[![Windows](https://img.shields.io/badge/Platform-Windows-blue?style=flat-square&logo=windows)](https://www.microsoft.com/windows)
[![Status](https://img.shields.io/badge/status-beta-yellow?style=flat-square)]()

**⚠️ BETA RELEASE – WORK IN PROGRESS ⚠️**  
This release introduces major features: physics, advanced materials, model import, and a fully-featured editor. Expect bugs and ongoing improvements.

**BinaxEngine** is a lightweight game engine built with C++ and OpenGL, featuring an integrated editor for scene management, physics simulation, and real-time rendering.

![BinaxEngine Editor](resources/screenshots/binaxscreenshot.png)

---
(Developed and maintained by a 14-year-old deleloper passionate about low-level systems and graphics)

## ✨ Features

### 🖥️ Editor & Workflow
- **ImGui-based interface** with docking, viewports, and custom themes
- **ImGuizmo integration** – translate, rotate, scale objects with snap support
- **Hierarchy panel** – parent-child object relationships
- **Inspector panel** – edit all components (transform, appearance, light, material, mesh, physics, outline)
- **Scene view** – camera navigation (WASD + mouse) with mouse capture
- **Content browser** – asset management (models, textures, shaders)
- **Menu bar** – file operations (save scene, exit), edit (duplicate, delete), view (wireframe, grid, gizmo, theme editor, VSync), physics (active physics, reset), skybox & shadows settings
- **Hotkeys** – Ctrl+S (save), Ctrl+D (duplicate), Delete (remove object), G (toggle grid), T/R/E (gizmo mode), X (gizmo local/world), ESC (release mouse)

### 🎨 Graphics & Rendering
- **OpenGL 4.6 core profile** with MSAA (4x)
- **PBR materials** – metallic, roughness, ambient occlusion, emission
- **Normal mapping** with adjustable strength
- **Texture support** – diffuse, normal, roughness, metallic, AO (load via file dialog)
- **UV scaling** and **World UV** projection (triplanar mapping)
- **Dynamic lights** – directional, point, spot (up to 8 active)
- **Shadow mapping** – directional light shadows with PCF (4/9 samples), adjustable bias and softness
- **Skybox** – cubemap-based environment (load 6 images)
- **Grid** – customizable white semi‑transparent grid with distance fade
- **Outline** – highlight selected objects (wireframe, vertices, fill)
- **Anisotropic filtering** for sharper textures at angles

### 🧠 Physics (Bullet 3.25)
- **Rigid body dynamics** – mass, gravity, collisions
- **Collider shapes** – Box, Sphere, Capsule
- **Material properties** – friction (0..1), restitution (0..1), rolling friction
- **Damping** – linear and angular drag
- **Static & dynamic bodies** – mass 0 = static (e.g., ground)
- **Physics controls** – "Active Physics" toggle, "Return" reset to initial positions
- **Real‑time synchronization** – transforms updated automatically

### 📦 Asset Import (Assimp)
- **Model formats** – OBJ, FBX, DAE, BLEND, 3DS, STL
- **Hierarchical import** – preserves object hierarchy
- **Material extraction** – loads diffuse, normal, roughness, metallic, AO textures
- **PBR parameter reading** – metallic/roughness factors from file
- **Mesh naming** – retains original mesh names

### 🎛️ Component System
- **Transform** – position, rotation, scale (with non‑uniform scale warning)
- **Appearance** – color, visibility
- **Light** – type, color, intensity, range, angle, direction
- **Material** – textures, metallic/roughness override, emission, UV settings
- **Mesh** – switch between primitives (cube, sphere, cylinder, cone, pyramid, plane)
- **Rendering** – cast/receive shadows
- **Physics Components** – add/remove RigidBody, choose collider type, adjust mass, friction, restitution, rolling friction, linear/angular damping
- **Camera** – FOV, near/far plane, switch active camera

### 🎥 Camera
- **First‑person controls** – WASD + mouse look
- **Multiple cameras** – switch via inspector
- **Adjustable FOV**, near/far planes

### 🛠️ Build & Integration
- **CMake** – easy project generation (Visual Studio 2022)
- **Static libraries** – GLFW, GLEW, Bullet, Assimp, ImGui, ImGuizmo, GLM, STB
- **Lightweight** – ~90 MB RAM, smooth performance

---

## 🆕 What's New in v0.5.0
- **Full physics integration** (Bullet) – RigidBody, colliders, friction, restitution, damping
- **PBR texture pipeline** – roughness, metallic, AO textures
- **Model import** (Assimp) – supports OBJ, FBX, DAE, BLEND, 3DS, STL with materials
- **Dynamic lights** – point and spot lights (in addition to directional)
- **Shadow mapping** – directional light shadows with PCF
- **Physics UI** – add/remove RigidBody, adjust mass, friction, etc. in inspector
- **Gizmo enhancements** – snap to grid, operation modes (translate/rotate/scale)
- **Outline selection** – wireframe, vertices, or fill highlight
- **Skybox editor** – load custom cubemap textures
- **Shadows settings** – toggle, bias, softness, PCF samples
- **Material improvements** – UV scale, world UV projection, emission
- **Normal mapping** – adjustable strength
- **Grid** – beautiful white grid with distance fade
- **Performance optimizations** – reduced memory footprint
- 
---

## 📥 Installation & Build

### Prerequisites
- Windows 10/11
- Visual Studio 2022 (with C++ development tools)
- CMake 3.15+
- Git

### Build Steps
```bash
git clone https://github.com/VladislavPim/BinaxEngine.git
cd BinaxEngine
mkdir build
cd build
cmake .. -A x64
cmake --build . --config Release
```

### Run
After build, execute `BinaxEngine.exe` from `build/Release/`.

---

## 🕹️ Editor Controls

| Action | Keys |
|--------|------|
| Move camera | WASD + Space/Shift |
| Rotate camera | Mouse (after Capture Mouse) |
| Capture/release mouse | Button in menu or ESC |
| Select object | Left-click in Hierarchy or Scene View |
| Delete object | Delete |
| Duplicate | Ctrl+D |
| Save scene | Ctrl+S |
| Gizmo mode | T (translate), R (rotate), E (scale) |
| Gizmo local/world | X |
| Toggle grid | G |

---

## 🧪 Example: Creating a Physics Scene

1. Add a **Cube** (via `+ Add Object → Cube`) – it gets a Box collider and is static by default.
2. Add a second **Cube**, move it up (e.g., Y=3).
3. Select the second cube, click **Add RigidBody**.
4. Enable physics via `Physics → Active Physics`.
5. The top cube will fall onto the bottom one and stop.

You can tweak mass, friction, restitution, and damping in the inspector.

---

## 📂 Project Structure

```
BinaxEngine/
├── assets/                 # shaders, textures, models
├── libs/                   # third‑party libraries (GLFW, GLEW, Bullet, Assimp, ImGui, GLM, STB)
├── resources/              # icons, fonts, skyboxes
├── src/
│   ├── Editor/             # ImGui editor code
│   ├── Graphics/           # OpenGL, shaders, meshes, materials, models, primitives, skybox
│   ├── Physics/            # Bullet wrapper
│   ├── Scene/              # GameObject, SceneManager, Camera
│   └── main.cpp            # entry point
├── CMakeLists.txt
└── README.md
```

---

## 🔮 Planned for Future Releases

- Audio system (OpenAL or SoLoud)
- Particle system
- Scene serialization (save/load to file)
- Scripting (Lua)
- Post-processing (bloom, HDR)
- Terrain system
- Animation (skeletal)

---

## 📝 License

This project is licensed under the MIT License – see the [LICENSE](LICENSE) file for details.

---
