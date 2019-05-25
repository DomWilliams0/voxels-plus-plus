project(voxels_test)

set(SOURCES test_world.cpp main.cpp catch.hpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} voxellib)

add_test(${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME})
