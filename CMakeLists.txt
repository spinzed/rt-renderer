cmake_minimum_required(VERSION 3.21)
project(rt-renderer LANGUAGES C CXX)

# Options for dependencies
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
option(ASSIMP_BUILD_SAMPLES OFF)
option(ASSIMP_BUILD_TESTS OFF)
option(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
option(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_OBJ_EXPORTER TRUE)
set(ASSIMP_BUILD_OBJ_IMPORTER TRUE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GLFW_BUILD_X11 TRUE)
set(GLFW_BUILD_WAYLAND FALSE)

# Include dependencies
add_subdirectory(dependencies/glfw)
add_subdirectory(dependencies/assimp)

# Optional CUDA support for the raytracer
option(ENABLE_CUDA "Enable CUDA support" ON)

if(ENABLE_CUDA)
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)
    message(STATUS "CUDA enabled")
    add_compile_definitions(ENABLE_CUDA=1)
else()
    add_compile_definitions(ENABLE_CUDA=0)
endif()

# Compiler-specific settings
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /std:c++20 /permissive-")
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od")  # /Od disables optimizations in Debug
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Zi") # /O2 for optimization in Release, /Zi for debug info
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/DEBUG /OPT:REF /OPT:ICF")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type -Wall -Wextra -Wpedantic -std=c++20")
    if(NOT WIN32)
        set(GLAD_LIBRARIES dl)
    endif()
endif()

include_directories(
    headers/
    dependencies/assimp/include/
    dependencies/glad/include/
    dependencies/glfw/include/
    dependencies/glm/
    dependencies/stb/
    dependencies/imgui/
)

add_library(imgui STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui/backends/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui/backends/imgui_impl_glfw.cpp
)

file(GLOB DEPENDENCY_SOURCES dependencies/glad/src/glad.c)
file(GLOB_RECURSE PROJECT_HEADERS headers/*.hpp headers/*.h)
file(GLOB_RECURSE PROJECT_SOURCES sources/*.cpp)
file(GLOB_RECURSE PROJECT_CUDA_SOURCES sources/*.cu)
file(GLOB PROJECT_SHADERS shaders/*.comp shaders/*.frag shaders/*.geom shaders/*.vert)
file(GLOB_RECURSE PROJECT_RESOURCES resources/*)

source_group("Headers" FILES ${PROJECT_HEADERS})
source_group("Shaders" FILES ${PROJECT_SHADERS})
source_group("Sources" FILES ${PROJECT_SOURCES})
source_group("Cuda" FILES ${PROJECT_CUDA_SOURCES})
source_group("Dependencies" FILES ${DEPENDENCY_SOURCES})
source_group("Resources" FILES ${PROJECT_RESOURCES})
set_source_files_properties(${PROJECT_RESOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_definitions(-DGLFW_INCLUDE_NONE -DPROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\" -DNOMINMAX)

if(ENABLE_CUDA AND CUDAToolkit_FOUND)
	add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_CUDA_SOURCES} ${PROJECT_HEADERS} ${PROJECT_SHADERS} ${DEPENDENCY_SOURCES} ${PROJECT_RESOURCES} ${PROJECT_CUDA_SOURCES})
    target_link_libraries(${PROJECT_NAME} CUDA::cudart)
else()
	add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_CUDA_SOURCES} ${PROJECT_HEADERS} ${PROJECT_SHADERS} ${DEPENDENCY_SOURCES} ${PROJECT_RESOURCES})
endif()

target_link_libraries(
    ${PROJECT_NAME}
    assimp
    glfw
    ${GLFW_LIBRARIES}
    ${GLAD_LIBRARIES}
    imgui
)

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    DEPENDS ${PROJECT_SHADERS}
)
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    DEPENDS ${PROJECT_RESOURCES}
)
