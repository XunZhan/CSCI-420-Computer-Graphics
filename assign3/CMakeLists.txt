cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 14)
set(PROJECT_NAME "hw3")
project(${PROJECT_NAME} CXX)
get_filename_component(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

include_directories(external/glm)
include_directories(external/imageIO)
include_directories(include)

# Header Files
set(HEADERS_FILES
        external/imageIO/imageIO.h
        external/imageIO/imageFormats.h
        )
source_group("Headers" FILES ${HEADERS_FILES})

# Source Files
set(SRC_FILES
        src/hw3.cpp
        external/imageIO/imageIO.cpp
        )
source_group("Sources" FILES ${SRC_FILES})

# Add executable to build.
add_executable(${PROJECT_NAME}
        ${HEADERS_FILES} ${SRC_FILES})

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

# 添加OpenGL头文件包含目录
target_include_directories(${PROJECT_NAME} PUBLIC ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})

# 让项目的可执行程序链接系统OpenGL库
target_link_libraries(${PROJECT_NAME} ${OPENGL_gl_LIBRARY} ${GLUT_glut_LIBRARY})

# link jpeg-9a-mac
set(JPEG9A_INCLUDE_DIR "${PROJECT_DIR}/external/jpeg-9a-mac/include")
set(JPEG9A_jpeg9a_LIBRARY "${PROJECT_DIR}/external/jpeg-9a-mac/lib/libjpeg.a")
target_include_directories(${PROJECT_NAME} PUBLIC ${JPEG9A_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${JPEG9A_jpeg9a_LIBRARY})