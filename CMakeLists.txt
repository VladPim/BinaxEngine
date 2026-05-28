cmake_minimum_required(VERSION 3.15)
project(BinaxEngine)
set(CMAKE_CXX_STANDARD 17)

# ========== ПУТИ ==========
set(LIBS_DIR "${CMAKE_SOURCE_DIR}/libs")
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
set(SHADERS_DIR "${ASSETS_DIR}/shaders")

# ========== BULLET ==========
set(BULLET_DIR "${LIBS_DIR}/bullet")
set(BULLET_INCLUDE "${BULLET_DIR}/src")
set(BULLET_LIB_DIR "${BULLET_DIR}/build/lib/Release")

set(BULLET_LIBS
    ${BULLET_LIB_DIR}/LinearMath.lib
    ${BULLET_LIB_DIR}/BulletCollision.lib
    ${BULLET_LIB_DIR}/BulletDynamics.lib
    ${BULLET_LIB_DIR}/BulletSoftBody.lib
)

if(EXISTS "${BULLET_LIB_DIR}/BulletDynamics.lib")
    message(STATUS "Bullet libraries found at ${BULLET_LIB_DIR}")
else()
    message(FATAL_ERROR "Bullet libraries not found at ${BULLET_LIB_DIR}")
endif()

# GLFW
set(GLFW_INCLUDE "${LIBS_DIR}/glfw/include")
set(GLFW_LIB "${LIBS_DIR}/glfw/lib-vc2022/glfw3.lib")

# GLEW
set(GLEW_INCLUDE "${LIBS_DIR}/glew/include")
set(GLEW_LIB "${LIBS_DIR}/glew/lib/Release/x64/glew32.lib")

# ImGui
set(IMGUI_DIR "${LIBS_DIR}/imgui")

# GLM
set(GLM_DIR "${LIBS_DIR}/glm")

# STB
set(STB_DIR "${LIBS_DIR}/stb")

# Assimp
set(ASSIMP_DIR "${LIBS_DIR}/assimp")
set(ASSIMP_INCLUDE "${ASSIMP_DIR}/include")
set(ASSIMP_LIB_DIR "${ASSIMP_DIR}/lib/x64")
find_library(ASSIMP_LIBRARY
    NAMES assimp-vc143-mt
    PATHS ${ASSIMP_LIB_DIR}
    NO_DEFAULT_PATH
)

# JSON (nlohmann)
set(JSON_INCLUDE "${LIBS_DIR}/json")

# ========== ПРОВЕРКИ ==========
if(NOT EXISTS "${GLFW_INCLUDE}/GLFW/glfw3.h")
    message(FATAL_ERROR "GLFW header not found at: ${GLFW_INCLUDE}")
endif()
if(NOT EXISTS "${GLEW_INCLUDE}/GL/glew.h")
    message(FATAL_ERROR "GLEW header not found at: ${GLEW_INCLUDE}")
endif()
if(NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
    message(FATAL_ERROR "ImGui not found at: ${IMGUI_DIR}")
endif()
if(NOT EXISTS "${GLM_DIR}/glm/glm.hpp")
    message(FATAL_ERROR "GLM not found at: ${GLM_DIR}")
endif()
if(NOT EXISTS "${STB_DIR}/stb_image.h")
    message(WARNING "stb_image.h not found at: ${STB_DIR}, textures won't work")
endif()
if(NOT EXISTS "${SHADERS_DIR}")
    message(WARNING "Shaders directory not found at: ${SHADERS_DIR}")
    file(MAKE_DIRECTORY "${SHADERS_DIR}")
endif()
if(NOT EXISTS "${ASSIMP_INCLUDE}/assimp/scene.h")
    message(WARNING "Assimp headers not found. Model loading disabled.")
endif()
if(NOT ASSIMP_LIBRARY)
    message(WARNING "Assimp library not found. Model loading disabled.")
endif()
if(NOT EXISTS "${JSON_INCLUDE}/nlohmann/json.hpp")
    message(FATAL_ERROR "JSON library not found at: ${JSON_INCLUDE}")
endif()

message(STATUS "=== Libraries found ===")
message(STATUS "GLFW: ${GLFW_INCLUDE}")
message(STATUS "GLEW: ${GLEW_INCLUDE}")
message(STATUS "ImGui: ${IMGUI_DIR}")
message(STATUS "GLM: ${GLM_DIR}")
message(STATUS "STB: ${STB_DIR}")
message(STATUS "Shaders: ${SHADERS_DIR}")
if(ASSIMP_LIBRARY)
    message(STATUS "Assimp: ${ASSIMP_LIBRARY}")
endif()
message(STATUS "JSON: ${JSON_INCLUDE}")

# ========== ИСХОДНЫЕ ФАЙЛЫ ==========
set(SOURCES
    src/main.cpp
    src/Graphics/Shader.cpp
    src/Graphics/Mesh.cpp
    src/Graphics/Primitives.cpp
    src/Graphics/Material.cpp
    src/Graphics/Skybox.cpp
    src/Graphics/Model.cpp
    src/Scene/GameObject.cpp
    src/Scene/SceneManager.cpp
    src/Scene/Camera.cpp
    src/Scene/Frustum.cpp
    src/Editor/EditorUI.cpp
    src/Physics/PhysicsWorld.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/ImGuizmo.cpp
)

# ========== РЕСУРСНЫЙ ФАЙЛ ==========
set(RESOURCES
    resources/binax_engine.rc
)

# ========== ИСПОЛНЯЕМЫЙ ФАЙЛ ==========
add_executable(BinaxEngine ${SOURCES} ${RESOURCES})

# ========== ВКЛЮЧАЕМ ДИРЕКТОРИИ ==========
target_include_directories(BinaxEngine PRIVATE
    src
    src/Editor
    src/Physics
    ${GLFW_INCLUDE}
    ${GLEW_INCLUDE}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLM_DIR}
    ${STB_DIR}
    ${ASSETS_DIR}
    ${BULLET_INCLUDE}
    ${JSON_INCLUDE}
)

if(EXISTS "${ASSIMP_INCLUDE}")
    target_include_directories(BinaxEngine PRIVATE ${ASSIMP_INCLUDE})
endif()

# ========== ОПРЕДЕЛЕНИЯ ==========
target_compile_definitions(BinaxEngine PRIVATE
    ASSETS_DIR="${ASSETS_DIR}"
)

message(STATUS "Bullet libraries: ${BULLET_LIBS}")
if(EXISTS "${BULLET_LIB_DIR}/BulletDynamics.lib")
    message(STATUS "BulletDynamics.lib found")
else()
    message(WARNING "BulletDynamics.lib NOT found")
endif()

# ========== БИБЛИОТЕКИ ==========
target_link_libraries(BinaxEngine PRIVATE
    ${GLFW_LIB}
    ${GLEW_LIB}
    opengl32.lib
    comdlg32.lib
    ${BULLET_LIBS}
)

if(ASSIMP_LIBRARY)
    target_link_libraries(BinaxEngine PRIVATE ${ASSIMP_LIBRARY})
endif()

# ========== КОПИРОВАНИЕ DLL И ФАЙЛОВ ==========
if(WIN32)
    set(GLFW_DLL "${LIBS_DIR}/glfw/lib-vc2022/glfw3.dll")
    if(EXISTS "${GLFW_DLL}")
        add_custom_command(TARGET BinaxEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GLFW_DLL}"
            $<TARGET_FILE_DIR:BinaxEngine>
        )
        message(STATUS "GLFW DLL will be copied")
    endif()

    set(GLEW_DLL "${LIBS_DIR}/glew/bin/Release/x64/glew32.dll")
    if(EXISTS "${GLEW_DLL}")
        add_custom_command(TARGET BinaxEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GLEW_DLL}"
            $<TARGET_FILE_DIR:BinaxEngine>
        )
        message(STATUS "GLEW DLL will be copied")
    endif()

    set(ASSIMP_DLL "${ASSIMP_DIR}/bin/x64/assimp-vc143-mt.dll")
    if(EXISTS "${ASSIMP_DLL}")
        add_custom_command(TARGET BinaxEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ASSIMP_DLL}"
            $<TARGET_FILE_DIR:BinaxEngine>
        )
        message(STATUS "Assimp DLL will be copied")
    endif()

    if(EXISTS "${SHADERS_DIR}")
        add_custom_command(TARGET BinaxEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SHADERS_DIR}"
            "$<TARGET_FILE_DIR:BinaxEngine>/assets/shaders"
        )
        message(STATUS "Shaders will be copied")
    endif()

    set(TEXTURES_DIR "${ASSETS_DIR}/textures")
    if(EXISTS "${TEXTURES_DIR}")
        add_custom_command(TARGET BinaxEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${TEXTURES_DIR}"
            "$<TARGET_FILE_DIR:BinaxEngine>/assets/textures"
        )
        message(STATUS "Textures will be copied")
    endif()
endif()

set(RESOURCES_DIR "${CMAKE_SOURCE_DIR}/resources")
if(EXISTS "${RESOURCES_DIR}")
    add_custom_command(TARGET BinaxEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${RESOURCES_DIR}"
        "$<TARGET_FILE_DIR:BinaxEngine>/resources"
    )
    message(STATUS "Resources will be copied")
endif()

message(STATUS "=== Binax Engine Configuration Complete ===")
