cmake_minimum_required(VERSION 3.5.0)
project(MiniEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")

set(TARGET_NAME MiniEngine)

file(GLOB_RECURSE SRC "./Internal/*.cpp")
set(GLAD_SRC "${CMAKE_SOURCE_DIR}/External/glad/src/glad.c")
file(GLOB_RECURSE IMGUI_SRC "./External/imgui/*.cpp")

set(GLFW3_INCLUDE_DIR "./External/glfw/include")
set(GLAD_INCLUDE_DIR "./External/glad/include")
set(IMGUI_INCLUDE_DIR "./External/imgui" )

set(TINYGLTF_INCLUDE_DIR "./External/tinygltf")
file(GLOB TINYGLTF_SRC "./External/tinygltf/tiny_gltf.cc")

set(TINYOBJ_INCLUDE_DIR "./External/tinyobjloader")
file(GLOB TINYOBJ_SRC "./External/tinygltf/tiny_obj_loader.cc")

set(ENTT_INCLUDE_DIR "./External/entt")

set(GLM_INCLUDE_DIR "./External/glm")

set(STB_INCLUDE_DIR "./External/stb")

set(FRAMEGRAPH_INCLUDE_DIR "./External/FrameGraph/include")
file(GLOB_RECURSE FRAMEGRAPH_SRC "./External/FrameGraph/*.cpp")

set(BVH_INCLUDE_DIR "./External/RadeonRays")
file(GLOB_RECURSE BVH_SRC_DIR "./External/RadeonRays/*.cpp")

set(GLFW3_LIB "${CMAKE_SOURCE_DIR}/External/glfw/lib-vc2022/glfw3.lib")

set(RENDERDOC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/External/renderdoc")

include_directories(
    "./Internal"
    "${GLFW3_INCLUDE_DIR}"
    "${GLAD_INCLUDE_DIR}"
    "${IMGUI_INCLUDE_DIR}"
    "${IMGUI_INCLUDE_DIR}/backends"
    "${TINYGLTF_INCLUDE_DIR}"
    "${TINYOBJ_INCLUDE_DIR}"
    "${ENTT_INCLUDE_DIR}"
    "${GLM_INCLUDE_DIR}"
    "${FRAMEGRAPH_INCLUDE_DIR}"
    "${BVH_INCLUDE_DIR}"
    "${RENDERDOC_DIR}"
    "${STB_INCLUDE_DIR}")

add_executable(${TARGET_NAME} 
    main.cpp
    ${SRC}
    ${IMGUI_SRC}
    ${GLAD_SRC}
    ${TINYGLTF_SRC}
    ${TINYOBJ_SRC}
    ${FRAMEGRAPH_SRC}
    ${BVH_SRC_DIR})

target_link_libraries(${TARGET_NAME} 
    ${GLFW3_LIB}
    "opengl32.lib")

# output dir
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ $<TARGET_FILE_DIR:${TARGET_NAME}>/Shaders
    COMMAND ${CMAKE_COMMAND} -E copy
    ${RENDERDOC_DIR}/renderdoc.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/renderdoc.dll)