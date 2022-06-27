cmake_minimum_required(VERSION 3.22)
project(computer_network C)

set(CMAKE_C_STANDARD 90)

add_library(computer_network common.c equipment.c server.c)

set(BUILD_SERVER server.c)
set(SOURCE_FILES_SERVER server.c)
add_executable(${BUILD_SERVER} ${SOURCE_FILES_SERVER})

set(BUILD_CLIENT equipment.c)
set(SOURCE_FILES_CLIENT equipment.c)
add_executable(${BUILD_CLIENT} ${SOURCE_FILES_CLIENT})
