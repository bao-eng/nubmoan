cmake_minimum_required(VERSION 3.15)
project(nubmoan C)

find_package(SDL2 REQUIRED)
find_package(cargs REQUIRED)

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} SDL2::SDL2 cargs::cargs)
