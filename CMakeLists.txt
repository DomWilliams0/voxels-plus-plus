cmake_minimum_required(VERSION 3.14)
project(voxels++)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(voxellib)
include_directories(voxellib/src)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} voxellib)
