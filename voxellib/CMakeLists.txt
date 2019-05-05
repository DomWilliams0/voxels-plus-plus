cmake_minimum_required(VERSION 3.14)
project(voxellib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -fno-stack-protector")

find_package(SFML 2.5 COMPONENTS graphics REQUIRED)
find_package(GLEW REQUIRED)

set(SOURCES
        game.cpp
        game.h
        world.cpp
        world.h
        error.h
        world_renderer.cpp
        world_renderer.h
        shader_loader.cpp
        shader_loader.h
        util.cpp
        util.h
        camera.cpp
        camera.h
        )

set(GLM_TEST_ENABLE off)
add_subdirectory(lib/glm)

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${SOURCES_LIB})
target_link_libraries(${PROJECT_NAME} sfml-graphics GLEW)
target_include_directories(${PROJECT_NAME} PUBLIC lib/glm)
