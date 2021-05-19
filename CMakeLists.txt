cmake_minimum_required(VERSION 3.19)
project(OpenGLAssignment)
set(CMAKE_CXX_STANDARD 14)

# set include directories, opengl directories
include_directories(include)
include_directories(C:\\OpenGLwrappers\\glm-0.9.7.5\\glm)
include_directories(C:\\OpenGLwrappers\\glew-1.10.0-win32\\glew-1.10.0\\include)
include_directories(C:\\OpenGLwrappers\\freeglut-MSVC-2.8.1-1.mp\\freeglut\\include)

# set link directories of glew, freeglut
link_directories(C:\\OpenGLwrappers\\glew-1.10.0-win32\\glew-1.10.0\\lib\\Release\\Win32)
link_directories(C:\\OpenGLwrappers\\freeglut-MSVC-2.8.1-1.mp\\freeglut\\lib)

aux_source_directory(src DIR_SRCS)
set(SOURCE_FILES ${DIR_SRCS})

# build sources
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# link .lib files
target_link_libraries(${PROJECT_NAME} glew32 opengl32)