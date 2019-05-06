cmake_minimum_required(VERSION 3.14)
project(voxel_test)

set(TESTS
        test_world.cpp
        )

add_executable(${PROJECT_NAME} ${TESTS} main.cpp catch.hpp)

include_directories(../voxellib)
target_link_libraries(${PROJECT_NAME} voxellib)

add_test(${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME})
