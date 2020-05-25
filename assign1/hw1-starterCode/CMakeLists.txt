# File generated at : 10:12:07, Wed 20 Nov 
# Converted Project : .\hw1.vcxproj
cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

##################### Variables ############################
# Change if you want modify path or other values           #
############################################################

# Project
get_filename_component(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
set(DEPENDENCIES_DIR ${PROJECT_DIR}/dependencies)
set(PROJECT_NAME "hw1")


# Outputs
set(OUTPUT_DEBUG ${PROJECT_DIR}/bin)
set(OUTPUT_RELEASE ${PROJECT_DIR}/bin)

################# CMake Project ############################
# The main options of project                              #
############################################################

project(${PROJECT_NAME} CXX)

# Define Release by default.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Build type not specified: Use Release by default.")
endif(NOT CMAKE_BUILD_TYPE)

############## Artefacts Output ############################
# Defines outputs , depending BUILD TYPE                   #
############################################################

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_DIR}/${OUTPUT_DEBUG}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_DIR}/${OUTPUT_DEBUG}")
    set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY "${PROJECT_DIR}/${OUTPUT_DEBUG}")
else()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_DIR}/${OUTPUT_RELEASE}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_DIR}/${OUTPUT_RELEASE}")
    set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY "${PROJECT_DIR}/${OUTPUT_RELEASE}")
endif()

# Messages
message("${PROJECT_NAME}: MAIN PROJECT: ${CMAKE_PROJECT_NAME}")
message("${PROJECT_NAME}: CURR PROJECT: ${CMAKE_CURRENT_SOURCE_DIR}")
message("${PROJECT_NAME}: CURR BIN DIR: ${CMAKE_CURRENT_BINARY_DIR}")

############### Files & Targets ############################
# Files of project and target to build                     #
############################################################

# Source Files
set(SRC_FILES
        ../hw1.cpp
        ../../openGLHelper-starterCode/basicPipelineProgram.cpp
        ../../openGLHelper-starterCode/openGLMatrix.cpp
        ../../openGLHelper-starterCode/pipelineProgram.cpp
        ../../external/imageIO/imageIO.cpp
        )
source_group("Sources" FILES ${SRC_FILES})

# Header Files
set(HEADERS_FILES
        ../../openGLHelper-starterCode/basicPipelineProgram.h
        ../../openGLHelper-starterCode/glutHeader.h
        ../../openGLHelper-starterCode/openGLHeader.h
        ../../openGLHelper-starterCode/openGLMatrix.h
        ../../openGLHelper-starterCode/pipelineProgram.h
        ../../external/imageIO/imageIO.h
        ../../external/imageIO/imageFormats.h
        )
source_group("Headers" FILES ${HEADERS_FILES})

# Add executable to build.
add_executable(${PROJECT_NAME}
        ${HEADERS_FILES} ${SRC_FILES}
        )

######################### Flags ############################
# Defines Flags for Windows and Linux                      #
############################################################

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
endif(NOT MSVC)

# Preprocessor definitions
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            -D_DEBUG
            -D_CONSOLE
            )
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE  /W3 /MD /MDd /Od /Zi /EHsc)
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            -DNDEBUG
            -D_CONSOLE
            )
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE  /W3 /Zi /EHsc)
    endif()
endif()

########### Link & Dependencies ############################
# Add project dependencies and Link to project             #
############################################################
include_directories(../external/glm)
include_directories(../external/imageIO)
include_directories(../openGLHelper-starterCode)

# link jpeg-9a-mac
set(JPEG9A_INCLUDE_DIR "${PROJECT_DIR}/../external/jpeg-9a-mac/include")
set(JPEG9A_jpeg9a_LIBRARY "${PROJECT_DIR}/../external/jpeg-9a-mac/lib/libjpeg.a")
target_include_directories(${PROJECT_NAME} PUBLIC ${JPEG9A_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${JPEG9A_jpeg9a_LIBRARY})

# 让CMake去系统搜索OpenGL库，找不到就报错(REQUIRED). Glut之后同理
# 找到了之后就会有两个变量可用：OPENGL_INCLUDE_DIR 和 OPENGL_gl_LIBRARY
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

# 添加OpenGL头文件包含目录
target_include_directories(${PROJECT_NAME} PUBLIC ${OPENGL_INCLUDE_DIR} ${GLUT_INCLUDE_DIR})

# 让项目的可执行程序链接系统OpenGL库
target_link_libraries(${PROJECT_NAME} ${OPENGL_gl_LIBRARY} ${GLUT_glut_LIBRARY})