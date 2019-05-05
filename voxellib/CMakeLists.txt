cmake_minimum_required(VERSION 3.14)
project(voxellib)

set(CMAKE_CXX_STANDARD 17)


find_package(SFML 2.5 COMPONENTS graphics REQUIRED)
add_library(${PROJECT_NAME} SHARED game.cpp game.h world.cpp world.h util.h error.h)
target_link_libraries(${PROJECT_NAME} sfml-graphics)