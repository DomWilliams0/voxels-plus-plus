cmake_minimum_required(VERSION 3.14)
project(voxellib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -fno-stack-protector")


find_package(SFML 2.5 COMPONENTS graphics REQUIRED)
find_package(GLEW REQUIRED)

add_library(${PROJECT_NAME} SHARED game.cpp game.h world.cpp world.h util.h error.h world_renderer.cpp world_renderer.h shader_loader.cpp shader_loader.h util.cpp)
target_link_libraries(${PROJECT_NAME} sfml-graphics GLEW)