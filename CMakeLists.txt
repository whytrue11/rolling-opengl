cmake_minimum_required(VERSION 3.19)
project(cw_c)

set(CMAKE_CXX_STANDARD 17)

add_executable(cw_c main.cpp)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(cw_c PRIVATE glfw)

find_package(glm CONFIG REQUIRED)
target_link_libraries(cw_c PRIVATE glm::glm)

find_package(GLEW REQUIRED)
target_link_libraries(cw_c PRIVATE GLEW::GLEW)

find_package(tinyobjloader CONFIG REQUIRED)
target_link_libraries(cw_c PRIVATE tinyobjloader::tinyobjloader)