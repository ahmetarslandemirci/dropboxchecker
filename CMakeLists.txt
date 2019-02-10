cmake_minimum_required(VERSION 2.7)

project(dropboxchecker C)
include(${CMAKE_SOURCE_DIR}/build/conanbuildinfo.cmake)
include_directories(${CONAN_INCLUDE_DIRS})

link_directories(${CONAN_LIB_DIRS})

add_executable(dropboxchecker main.c)
add_definitions(${CONAN_DEFINES})
target_link_libraries(dropboxchecker ${CONAN_LIBS} )


