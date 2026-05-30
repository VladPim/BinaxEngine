# 🎮 BinaxEngine - Game Engine (v0.7.0)

[![CMake](https://img.shields.io/badge/CMake-3.15+-blue?style=flat-square&logo=cmake)](https://cmake.org/)
[![C++](https://img.shields.io/badge/C++-17-orange?style=flat-square&logo=cplusplus)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-3.3-red?style=flat-square&logo=opengl)](https://www.opengl.org/)
[![Bullet Physics](https://img.shields.io/badge/Bullet-3.25-brightgreen?style=flat-square)](https://pybullet.org/)
[![Assimp](https://img.shields.io/badge/Assimp-5.2-blue?style=flat-square)](https://assimp.org/)
[![Windows](https://img.shields.io/badge/Platform-Windows-blue?style=flat-square&logo=windows)](https://www.microsoft.com/windows)
[![Status](https://img.shields.io/badge/status-beta-yellow?style=flat-square)]()

---

![BinaxEngine Editor](resources/screenshots/binaxscreenshot.png)

---
*(Developed and maintained by a 14‑year‑old developer passionate about low‑level systems and graphics)*

## ✨ Features

### 🖥️ Editor & Workflow
- **ImGui-based interface** with docking, viewports, and custom themes
- **ImGuizmo integration** – translate, rotate, scale objects with snap support
- **Hierarchy panel** – parent-child object relationships with drag‑and‑drop
- **Inspector panel** – edit all components (transform, appearance, light, material, mesh, physics, **audio**, camera)
- **Scene view** – camera navigation (WASD + mouse) with mouse capture
- **Content browser** – asset management (models, textures, shaders, audio files)
- **Menu bar** – file operations, edit (duplicate, delete), view (wireframe, grid, gizmo, theme editor, VSync), physics (active physics, reset), skybox & shadows settings
- **Hotkeys** – Ctrl+S (save), Ctrl+D (duplicate), Delete (remove object), G (toggle grid), T/R/E (gizmo mode), X (gizmo local/world), ESC (release mouse)

### 🎨 Graphics & Rendering
- **OpenGL 3.3 core profile**
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
- **Visual gizmos** – camera direction (white pyramid), light range (sphere for point, cone for spot), audio range (sphere), all toggleable per object

### 🔊 Audio (New in v0.7.0!)
- **Miniaudio backend** – single‑header, supports WAV, MP3, FLAC, OGG
- **Audio Source component** – add/remove via "Add Component" menu
- **2D / 3D spatial audio** – 3D sound with distance attenuation
- **Real‑time parameter updates** – volume, loop, min/max distance, 3D toggle – without restarting playback
- **Visual gizmo** – white wireframe sphere showing max distance (toggleable)
- **Listener** – automatically attached to the active camera
- **Performance** – efficient sound slot management (up to 256 simultaneous sounds)

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
- **Physics** – add/remove RigidBody, choose collider type, adjust mass, friction, restitution, rolling friction, linear/angular damping
- **Audio Source** – load audio file, volume, loop, 3D toggle, min/max distance, play/stop, show gizmo
- **Camera** – FOV, near/far plane, switch active camera, show frustum gizmo

### 🎥 Camera
- **First‑person controls** – WASD + mouse look
- **Multiple cameras** – switch via inspector
- **Adjustable FOV**, near/far planes
- **Visual frustum gizmo** (white pyramid) for non‑active cameras

### ⚡ Performance & System
- **Optimised gizmo rendering** – precomputed geometry, batched draw calls, `glBufferSubData` for dynamic updates
- **Administrator rights request** – reduces idle CPU usage from ~8‑10% to ~3% (tested)
- **Low memory footprint** – ~90 MB RAM

---

## 🆕 What's New in v0.7.0
- **Full audio system** (miniaudio) – 2D/3D sound, MP3/WAV/FLAC/OGG support
- **Audio Source component** – add/remove via component menu, real‑time parameter updates without restart
- **Audio distance gizmo** – white wireframe sphere (toggleable)
- **Optimised gizmo rendering** – precomputed circles, batched buffers, no per‑frame trig
- **Administrator rights prompt** – reduces CPU idle load (3% vs 8‑10%)
- **Improved component UI** – "Add Audio Source" inside the Add Component popup alongside Physics
- **Audio listener** – follows active camera for 3D panning
- **Real‑time volume/min/max distance updates** for playing sounds

## 🐛 Fixes & Improvements (v0.7.0)
- Fixed `min`/`max` macro conflicts on Windows (added `NOMINMAX`)
- Fixed `miniaudio` linking – added `winmm.lib` and `ole32.lib`
- Corrected missing audio component in inspector
- Gizmos now respect show/hide toggles per object

---

## 📥 Installation & Build

### Prerequisites
- Windows 10/11
- Visual Studio 2022 (with C++ development tools)
- CMake 3.15+
- Git

## Build Steps
```bash
git clone https://github.com/VladPim/BinaxEngine.git
cd BinaxEngine
mkdir build
cd build
cmake .. -A x64
cmake --build . --config Release
```

### Run
After build, execute `BinaxEngine.exe` from `build/Release/`.

---
🕹️ Editor Controls
Action	Keys
Move camera	WASD + Space/Shift
Rotate camera	Mouse (after Capture Mouse)
Capture/release mouse	Button in menu or ESC
Select object	Left-click in Hierarchy or Scene View
Delete object	Delete
Duplicate	Ctrl+D
Save scene	Ctrl+S
Gizmo mode	T (translate), R (rotate), E (scale)
Gizmo local/world	X
Toggle grid	G
🧪 Example: Creating a Physics Scene
Add a Cube (via + Add Object → Cube) – it gets a Box collider and is static by default.

Add a second Cube, move it up (e.g., Y=3).

Select the second cube, click Add Component → Physics.

Enable physics via Physics → Active Physics.

The top cube will fall onto the bottom one and stop.

You can tweak mass, friction, restitution, and damping in the inspector.

🔊 Example: Adding 3D Sound
Create an empty object (+ Add Object → Empty) or use an existing one.

Select it, go to Inspector → Components → Add Component → Audio Source.

Load a sound file (WAV, MP3, etc.) via the ... button.

Enable 3D Sound, set Min Distance (e.g., 1) and Max Distance (e.g., 20).

Press Play – move the camera closer/farther to hear the attenuation.

Enable Show Gizmo (Range) to visualise the max distance sphere.

📂 Project Structure
```
BinaxEngine/
├── assets/                 # shaders, textures, models
├── libs/                   # third‑party libraries (GLFW, GLEW, Bullet, Assimp, ImGui, GLM, STB, miniaudio)
├── resources/              # icons, fonts, skyboxes
├── src/
│   ├── Audio/              # miniaudio wrapper
│   ├── Editor/             # ImGui editor code
│   ├── Graphics/           # OpenGL, shaders, meshes, materials, models, primitives, skybox
│   ├── Physics/            # Bullet wrapper
│   ├── Scene/              # GameObject, SceneManager, Camera, Frustum
│   └── main.cpp            # entry point
├── CMakeLists.txt
└── README.md
```
---

🔮 Planned for Future Releases
Particle system

Scene serialization (save/load to file)

Scripting (Lua)

Post-processing (bloom, HDR)

Terrain system

Animation (skeletal)

More audio features (attenuation curves, Doppler effect)

---

📝 License
This project is licensed under the MIT License – see the LICENSE file for details.

---
