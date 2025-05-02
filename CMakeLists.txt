#Template for INFOH502
#Heavily inspired from https://github.com/Polytonic/Glitter/blob/master/CMakeLists.txt

#project setup
project(infoh502-cpp_Project)

#set the minimal version of cmake and the minimal version of the c++ language
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 14)     
set(CMAKE_VERBOSE_MAKEFILE ON)

#These commands are there to specify the path to the folder containing the object and textures files as macro
#With these you can just use PATH_TO_OBJECTS and PATH_TO_TEXTURE in your c++ code and the compiler will replace it by the correct expression
add_compile_definitions(PATH_TO_OBJECTS="${CMAKE_CURRENT_SOURCE_DIR}/objects")
add_compile_definitions(PATH_TO_TEXTURE="${CMAKE_CURRENT_SOURCE_DIR}/textures")
add_compile_definitions(PATH_TO_SHADERS="${CMAKE_CURRENT_SOURCE_DIR}/shaders")

# #define some variable
# set(COMPILE_LAB01 OFF CACHE BOOL "Compile the first Lab")
# set(COMPILE_LAB02 OFF CACHE BOOL "Compile the second Lab")
# set(COMPILE_LAB03 ON CACHE BOOL "Compile the third Lab")
# set(COMPILE_LAB04 OFF CACHE BOOL "Compile the fourth Lab")
# set(COMPILE_SOLUTIONS OFF CACHE BOOL "Compile the solutions")


find_package(OpenGL REQUIRED)

#for glad library
add_library( glad STATIC 3rdParty/glad/src/glad.c)
set(GLAD_INCLUDE "3rdParty/glad/include")
target_include_directories(glad PUBLIC ${GLAD_INCLUDE})

# #for GLFW
# option(GLFW_BUILD_DOCS OFF)
# option(GLFW_BUILD_EXAMPLES OFF)
# option(GLFW_BUILD_TESTS OFF)

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

                 
# if(COMPILE_LAB01)
#     add_subdirectory(LAB01)
# endif()

# if(COMPILE_LAB02)
# 	add_subdirectory(LAB02)
# endif()

# if(COMPILE_LAB03)
# 	add_subdirectory(LAB03)
# endif()

#================================

# project("LAB01")

#Put the sources into a variable
set(SOURCES_MAIN "main.cpp" "./utils/shader.h" "./utils/object.h" "./utils/camera.h")
set(SOURCES_MAIN2 "multip_moai_text.cpp" "./utils/shader.h" "./utils/object.h" "./utils/camera.h")
set(SOURCES_LIGHT "light.cpp" "./utils/shader.h" "./utils/object.h" "./utils/camera.h")
set(SOURCES_SUN "sun_sim.cpp" "./utils/shader.h" "./utils/object.h" "./utils/camera.h")
set(SOURCES_MAP "cubeMap.cpp" "./utils/shader.h" "./utils/object.h" "./utils/camera.h")
set(SOURCES_SHADOW "shadow.cpp" "./utils/shader.h" "./utils/object.h" "./utils/camera.h")



add_compile_definitions(PATH_TO_LAB1_SHADERS="${CMAKE_CURRENT_SOURCE_DIR}")

#To use the content of a variable you need to use ${NAME_OF_YOUR_VARIABLE}
#Specify that you want to generate an executable with a certain name using a set of sources
add_executable(${PROJECT_NAME}_ex01 ${SOURCES_MAIN})
#Specify which libraries you want to use with your executable
target_link_libraries(${PROJECT_NAME}_ex01 PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex02 ${SOURCES_MAIN2})
#Specify which libraries you want to use with your executable
target_link_libraries(${PROJECT_NAME}_ex02 PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_light ${SOURCES_LIGHT})
#Specify which libraries you want to use with your executable
target_link_libraries(${PROJECT_NAME}_light PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_sun_sim ${SOURCES_SUN})
#Specify which libraries you want to use with your executable
target_link_libraries(${PROJECT_NAME}_sun_sim PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_cubmap ${SOURCES_MAP})
#Specify which libraries you want to use with your executable
target_link_libraries(${PROJECT_NAME}_cubmap PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_shadow ${SOURCES_SHADOW})
#Specify which libraries you want to use with your executable
target_link_libraries(${PROJECT_NAME}_shadow PUBLIC OpenGL::GL glfw glad)