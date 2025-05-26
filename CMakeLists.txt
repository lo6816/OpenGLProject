#Template for INFOH502
#Heavily inspired from https://github.com/Polytonic/Glitter/blob/master/CMakeLists.txt

# === FreeType ===
find_path(FREETYPE_INCLUDE_DIR ft2build.h PATHS /opt/homebrew/opt/freetype/include/freetype2)
find_path(FREETYPE_INCLUDE_DIR ft2build.h PATHS
    /opt/homebrew/opt/freetype/include
    /opt/homebrew/Cellar/freetype/include
    /opt/homebrew/opt/freetype/include/freetype2
)

find_library(FREETYPE_LIBRARY freetype PATHS /opt/homebrew/opt/freetype/lib)


if (FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARY)
    message(STATUS "FreeType found")
    include_directories(${FREETYPE_INCLUDE_DIR})
    set(HAS_FREETYPE TRUE)
else()
    message(FATAL_ERROR "FreeType not found")
endif()

#project setup
project(infoh502-cpp_Project)

#set the minimal version of cmake and the minimal version of the c++ language
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
# set(CMAKE_CXX_STANDARD 14)     
# set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#These commands are there to specify the path to the folder containing the object and textures files as macro
#With these you can just use PATH_TO_OBJECTS and PATH_TO_TEXTURE in your c++ code and the compiler will replace it by the correct expression
add_compile_definitions(PATH_TO_OBJECTS="${CMAKE_CURRENT_SOURCE_DIR}/objects")
add_compile_definitions(PATH_TO_TEXTURE="${CMAKE_CURRENT_SOURCE_DIR}/textures")
add_compile_definitions(PATH_TO_SHADERS="${CMAKE_CURRENT_SOURCE_DIR}/shaders")

find_package(OpenGL REQUIRED)

#for glad library
add_library( glad STATIC 3rdParty/glad/src/glad.c)
set(GLAD_INCLUDE "3rdParty/glad/include")
target_include_directories(glad PUBLIC ${GLAD_INCLUDE})

#Search for glfw in case it is installed
find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
    #if glfw is not found, we will use the sources from the submodules
    add_subdirectory(3rdParty/glfw)
    include_directories(3rdParty/glfw/include/)
endif()



include_directories(3rdParty/glad/include/
                    3rdParty/glm/
                    3rdParty/stb/)

#================================

#Put the sources into a variable
set(SOURCES_MAIN "main.cpp" "./utils/particles.h" "./utils/shader.h" "./utils/object.h" "./utils/camera.h" "./utils/text.h" "./utils/picking.h")

add_compile_definitions(PATH_TO_LAB1_SHADERS="${CMAKE_CURRENT_SOURCE_DIR}")

#To use the content of a variable you need to use ${NAME_OF_YOUR_VARIABLE}
#Specify that you want to generate an executable with a certain name using a set of sources
add_executable(${PROJECT_NAME}_main ${SOURCES_MAIN})
#Specify which libraries you want to use with your executable
target_link_libraries(${PROJECT_NAME}_main PUBLIC OpenGL::GL glfw glad ${FREETYPE_LIBRARY} )
