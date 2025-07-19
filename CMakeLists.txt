cmake_minimum_required(VERSION 3.16)
project(RaytracingWindows)

set(CMAKE_CXX_STANDARD 17)

# GLFW
find_package(OpenGL REQUIRED)
add_subdirectory(external/glfw)

# GLAD
add_library(glad "external/glad/src/glad.c")
target_include_directories(glad PUBLIC "external/glad/include")
include_directories(glm)

# Executable
add_executable(RaytracingWindows main.cpp)
target_link_libraries(RaytracingWindows glfw glad OpenGL::GL)
target_include_directories(RaytracingWindows PRIVATE external/glad/include)


